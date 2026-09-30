// Copyright (c) 2026 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cxx/ast.h>
#include <cxx/attributes.h>
#include <cxx/binder.h>
#include <cxx/control.h>
#include <cxx/dependent_types.h>
#include <cxx/memory_layout.h>
#include <cxx/names.h>
#include <cxx/symbols.h>
#include <cxx/type_traits.h>
#include <cxx/types.h>
#include <cxx/util.h>
#include <cxx/views/symbols.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace cxx {
[[nodiscard]] static auto virtualBasesInInheritanceGraphOrder(
    ClassSymbol* classSymbol) -> std::vector<ClassSymbol*> {
  struct InheritanceFrame {
    ClassSymbol* classSymbol;
    std::size_t nextBase = 0;
  };

  std::vector<ClassSymbol*> virtualBases;
  std::vector<InheritanceFrame> frames{{classSymbol}};
  while (!frames.empty()) {
    auto& frame = frames.back();
    auto& bases = frame.classSymbol->baseClasses();
    if (frame.nextBase == bases.size()) {
      frames.pop_back();
      continue;
    }
    auto base = bases[frame.nextBase++];
    auto baseClass = resolved_base_class(base);
    if (!baseClass) continue;
    if (base->isVirtual()) {
      if (std::ranges::contains(virtualBases, baseClass)) continue;
      virtualBases.push_back(baseClass);
    }
    if (is_laid_out_without_virtual_base_subobjects(baseClass)) continue;
    frames.push_back({baseClass});
  }
  return virtualBases;
}

[[nodiscard]] static auto indirectPrimaryBasesOf(ClassSymbol* classSymbol)
    -> std::vector<ClassSymbol*> {
  std::vector<ClassSymbol*> primaryBases;
  std::vector<ClassSymbol*> visited{classSymbol};

  for (std::size_t index = 0; index < visited.size(); ++index) {
    for (auto baseClass : visited[index]->baseClasses()) {
      auto base = resolved_base_class(baseClass);
      if (!base) continue;
      if (std::ranges::contains(visited, base)) continue;
      visited.push_back(base);

      auto baseLayout = base->layout();
      if (!baseLayout || !baseLayout->primaryBaseIsVirtual()) continue;
      if (std::ranges::contains(primaryBases, baseLayout->primaryBase()))
        continue;
      primaryBases.push_back(baseLayout->primaryBase());
    }
  }

  return primaryBases;
}

struct [[nodiscard]] Binder::BuildRecordLayout {
  Binder& binder;
  ClassSymbol* classSymbol;
  const MemoryLayout* memoryLayout;
  std::unique_ptr<ClassLayout> layout;

  int calculatedSize = 0;
  int calculatedAlignment = 1;
  std::uint64_t runningSizeof = 0;
  std::uint64_t emptyComponentEnd = 0;
  std::uint64_t emittedEnd = 0;
  std::uint32_t currentIndex = 0;

  std::uint64_t nextBitPos = 0;
  int runStartByte = 0;
  std::uint32_t runIndex = 0;
  bool inBitfieldRun = false;
  std::vector<FieldSymbol*> runFields;
  std::multimap<std::uint64_t, EmptySubobjectComponent> placedComponents;
  std::uint64_t maxPlacedComponentSpan = 0;
  std::uint64_t maxPlacedSubobjectOffset = 0;
  using EmptySubobjectWalk =
      std::vector<std::pair<const EmptySubobjectComponent*, std::uint64_t>>;
  EmptySubobjectWalk placedWalk;
  EmptySubobjectWalk candidateWalk;
  std::vector<std::pair<ClassSymbol*, ClassLayout::MemberInfo>>
      indirectPrimaryPlacements;

  struct AllocatedBase {
    ClassSymbol* symbol = nullptr;
    bool isVirtual = false;

    [[nodiscard]] auto operator==(const AllocatedBase&) const -> bool = default;
  };

  std::vector<std::pair<ClassSymbol*, AllocatedBase>> indirectPrimaryOwners;

  int packValue = 0;

  BuildRecordLayout(Binder& b, ClassSymbol* cls)
      : binder(b),
        classSymbol(cls),
        memoryLayout(b.control()->memoryLayout()),
        layout(std::make_unique<ClassLayout>()) {
    packValue = cls->packAlignment();
  }

  auto control() const -> Control* { return binder.control(); }

  auto operator()() -> std::expected<bool, std::string>;
  auto validate() -> std::expected<bool, std::string>;
  void completeFieldTypes();
  [[nodiscard]] auto computeAbiEmpty() const -> bool;
  [[nodiscard]] auto hasOnlyZeroSizeDataMembers(ClassSymbol* candidate) const
      -> bool;
  [[nodiscard]] auto isNearlyEmptyClass(ClassSymbol* classSymbol) const -> bool;
  [[nodiscard]] auto selectPrimaryBase() const -> std::pair<ClassSymbol*, bool>;
  void padTo(std::uint64_t offset);
  void layoutVtable();
  void layoutBases();
  void layoutVirtualBases();
  void recordIndirectPrimaryPlacement(ClassSymbol* primary,
                                      ClassLayout::MemberInfo info);
  [[nodiscard]] auto indirectPrimaryPlacement(ClassSymbol* primary) const
      -> std::optional<ClassLayout::MemberInfo>;
  void assignIndirectPrimaryOwners();
  [[nodiscard]] auto isIndirectPrimary(ClassSymbol* base) const -> bool;
  void assignIndirectPrimaryOwner(ClassSymbol* cls, AllocatedBase owner);
  [[nodiscard]] auto ownsIndirectPrimary(AllocatedBase owner,
                                         ClassSymbol* primary) const -> bool;
  using IndirectPrimaryOffsets =
      std::vector<std::pair<ClassSymbol*, std::uint64_t>>;
  void appendPrimaryChain(IndirectPrimaryOffsets& primaries, ClassSymbol* cls,
                          AllocatedBase owner, std::uint64_t offset) const;
  [[nodiscard]] auto indirectPrimariesAllocatedWith(AllocatedBase root) const
      -> IndirectPrimaryOffsets;
  [[nodiscard]] auto baseNonVirtualSize(ClassSymbol* base) -> std::uint64_t;
  [[nodiscard]] auto allocateBaseSubobject(ClassSymbol* base, bool isVirtual)
      -> ClassLayout::MemberInfo;
  void growSizeof(std::uint64_t offset, std::uint64_t sizeInBytes);
  void recordEmptyComponent(std::uint64_t offset, std::uint64_t sizeInBytes);
  [[nodiscard]] static auto elementDisplacement(
      const EmptySubobjectComponent& component, std::uint64_t displacement)
      -> std::optional<std::uint64_t>;

  [[nodiscard]] auto hasEmptySubobjectAt(const EmptySubobjectComponent& placed,
                                         ClassSymbol* symbol,
                                         std::uint64_t address) -> bool;

  [[nodiscard]] auto placedAt(ClassSymbol* symbol, std::uint64_t address)
      -> bool;

  [[nodiscard]] auto conflictsAt(const EmptySubobjectComponent& candidate,
                                 std::uint64_t offset) -> bool;

  [[nodiscard]] auto conflictsAt(
      const std::vector<EmptySubobjectComponent>& candidates,
      std::uint64_t offset) -> bool;

  [[nodiscard]] auto classSubobjectOffset(
      const std::vector<EmptySubobjectComponent>& candidates, bool tryZero,
      std::uint64_t alignment) -> std::uint64_t;
  void appendCandidate(std::vector<EmptySubobjectComponent>& candidates,
                       ClassSymbol* symbol, bool isCompleteObject,
                       std::uint64_t offset, ClassSubobjectExtent extent = {});
  void recordClassSubobject(ClassSymbol* symbol, bool isCompleteObject,
                            std::uint64_t offset,
                            ClassSubobjectExtent extent = {});
  auto layoutFields() -> std::expected<bool, std::string>;
  auto layoutBitfield(FieldSymbol* field) -> std::expected<bool, std::string>;
  [[nodiscard]] auto regularFieldOffset(FieldSymbol* field,
                                        bool isEmptyDataMember)
      -> std::uint64_t;
  void layoutZeroWidthBitfield(FieldSymbol* field);
  auto layoutRegularField(FieldSymbol* field)
      -> std::expected<bool, std::string>;
  void closeBitfieldRun();
  [[nodiscard]] auto isPackedClass() const -> bool;
  [[nodiscard]] auto packAlignment(int alignment) const -> int;
  [[nodiscard]] auto keepsBitFieldInAllocationUnit(FieldSymbol* field) const
      -> bool;
  void propagateBaseFields();
  void propagateAnonymousFields();
  void copyFieldInfos(const ClassLayout* ownerLayout, std::uint64_t ownerOffset,
                      std::uint32_t containingIndex);
  void finalize();
  [[nodiscard]] auto excessWidthContainer(int bitWidth) const -> const Type*;
  void buildVTableLayout();
};

auto Binder::buildRecordLayout(ClassSymbol* classSymbol)
    -> std::expected<bool, std::string> {
  return BuildRecordLayout{*this, classSymbol}();
}

auto Binder::BuildRecordLayout::operator()()
    -> std::expected<bool, std::string> {
  if (auto status = validate(); !status) return status;

  completeFieldTypes();

  layout->setAbiEmpty(computeAbiEmpty());
  layoutVtable();
  assignIndirectPrimaryOwners();
  layoutBases();

  auto fieldsStatus = layoutFields();
  if (!fieldsStatus) return fieldsStatus;
  if (!fieldsStatus.value()) return false;

  auto nonVirtualSize = static_cast<std::uint64_t>(calculatedSize);
  if (emptyComponentEnd > nonVirtualSize) nonVirtualSize = emptyComponentEnd;

  layout->setNonVirtualSize(nonVirtualSize);
  layout->setNonVirtualAlignment(calculatedAlignment);

  layoutVirtualBases();

  propagateBaseFields();
  finalize();

  return true;
}

auto Binder::BuildRecordLayout::validate() -> std::expected<bool, std::string> {
  for (auto base : classSymbol->baseClasses()) {
    auto baseClassSymbol = symbol_cast<ClassSymbol>(base->symbol());
    if (!baseClassSymbol) {
      return std::unexpected(std::format("base class '{}' not found",
                                         to_diag_string(base->name())));
    }
    if (!baseClassSymbol->isComplete()) {
      binder.traits.require_complete_class(baseClassSymbol);
    }
    baseClassSymbol = baseClassSymbol->resolvedDefinition();
    if (!baseClassSymbol->isComplete()) {
      return std::unexpected(
          std::format("base class '{}' is incomplete",
                      to_diag_string(baseClassSymbol->name())));
    }
  }
  return true;
}

void Binder::BuildRecordLayout::completeFieldTypes() {
  for (auto field : views::members(classSymbol) | views::non_static_fields) {
    auto fieldElementType =
        binder.traits.remove_cv(binder.traits.remove_all_extents(
            binder.traits.remove_cv(field->type())));

    auto classType = type_cast<ClassType>(fieldElementType);
    if (!classType) continue;

    binder.traits.require_complete_class(classType->symbol());

    if (auto alignment =
            binder.control()->memoryLayout()->alignmentOf(field->type())) {
      field->setAlignment(alignment.value());
    }
  }
}

auto Binder::BuildRecordLayout::hasOnlyZeroSizeDataMembers(
    ClassSymbol* candidate) const -> bool {
  return std::ranges::all_of(
      views::members(candidate) | views::non_static_fields,
      [this](FieldSymbol* field) {
        return binder.traits.is_zero_size_subobject(field);
      });
}

auto Binder::BuildRecordLayout::computeAbiEmpty() const -> bool {
  if (views::any_function(classSymbol->members(),
                          [](FunctionSymbol* f) { return f->isVirtual(); }))
    return false;

  if (!hasOnlyZeroSizeDataMembers(classSymbol)) return false;

  for (auto base : classSymbol->baseClasses()) {
    if (base->isVirtual()) return false;
    auto baseClass = symbol_cast<ClassSymbol>(base->symbol());
    if (!baseClass) return false;
    auto baseLayout = baseClass->resolvedDefinition()->layout();
    if (!baseLayout || !baseLayout->isAbiEmpty()) return false;
  }
  return true;
}

void Binder::BuildRecordLayout::layoutVtable() {
  if (classSymbol->isUnion()) return;

  const auto hasVirtualFunction = views::any_function(
      classSymbol->members(), [](FunctionSymbol* f) { return f->isVirtual(); });
  const auto hasDynamicBase = std::ranges::any_of(
      classSymbol->baseClasses(), [](BaseClassSymbol* base) {
        auto baseClass = resolved_base_class(base);
        return baseClass && baseClass->layout() &&
               baseClass->layout()->hasVtable();
      });
  const auto hasVirtualBase = std::ranges::any_of(
      classSymbol->baseClasses(),
      [](BaseClassSymbol* base) { return base->isVirtual(); });
  if (!hasVirtualFunction && !hasDynamicBase && !hasVirtualBase) return;

  layout->setHasVtable(true);
  auto [primaryBase, primaryIsVirtual] = selectPrimaryBase();
  if (primaryBase) {
    layout->setPrimaryBase(primaryBase, primaryIsVirtual);
    if (!primaryIsVirtual) return;

    ClassLayout::MemberInfo primaryInfo;
    primaryInfo.index = currentIndex++;
    layout->setVirtualBaseInfo(primaryBase, primaryInfo);
    layout->setHasDirectVtable(true);
    layout->setVtableIndex(primaryInfo.index);
  } else {
    layout->setHasDirectVtable(true);
    layout->setVtableIndex(currentIndex++);
  }

  auto ptrSize = static_cast<int>(memoryLayout->sizeOfPointer());
  calculatedSize = ptrSize;
  calculatedAlignment = packAlignment(isPackedClass() ? 1 : ptrSize);
  emittedEnd = static_cast<std::uint64_t>(ptrSize);
  nextBitPos = static_cast<std::uint64_t>(calculatedSize) * 8;
}

auto Binder::BuildRecordLayout::isNearlyEmptyClass(ClassSymbol* candidate) const
    -> bool {
  if (!candidate) return false;
  auto candidateLayout = candidate->resolvedDefinition()->layout();
  if (!candidateLayout || !candidateLayout->hasVtable()) return false;
  return candidateLayout->nonVirtualSize() == memoryLayout->sizeOfPointer();
}

auto Binder::BuildRecordLayout::selectPrimaryBase() const
    -> std::pair<ClassSymbol*, bool> {
  for (auto base : classSymbol->baseClasses()) {
    if (base->isVirtual()) continue;
    auto baseClass = resolved_base_class(base);
    if (baseClass && baseClass->layout() && baseClass->layout()->hasVtable())
      return {baseClass, false};
  }

  std::vector<ClassSymbol*> candidates;
  for (auto virtualBase : virtualBasesInInheritanceGraphOrder(classSymbol)) {
    if (isNearlyEmptyClass(virtualBase)) candidates.push_back(virtualBase);
  }
  if (candidates.empty()) return {nullptr, false};

  auto indirectPrimaryBases = indirectPrimaryBasesOf(classSymbol);

  auto candidate = std::ranges::find_if(candidates, [&](ClassSymbol* base) {
    return !std::ranges::contains(indirectPrimaryBases, base);
  });
  if (candidate != candidates.end()) return {*candidate, true};
  if (!candidates.empty()) return {candidates.front(), true};
  return {nullptr, false};
}

auto Binder::BuildRecordLayout::baseNonVirtualSize(ClassSymbol* base)
    -> std::uint64_t {
  if (!base->layout()) return base->sizeInBytes();
  return binder.traits.non_virtual_size(base->type());
}

auto Binder::fieldElementClass(FieldSymbol* field) -> ClassSymbol* {
  auto elementType = traits.remove_cv(traits.remove_all_extents(field->type()));
  auto classType = type_cast<ClassType>(elementType);
  if (!classType || !classType->symbol()) return nullptr;
  return classType->symbol()->resolvedDefinition();
}

auto Binder::fieldArrayExtent(FieldSymbol* field) -> ClassSubobjectExtent {
  ClassSubobjectExtent extent;
  if (!traits.is_array(field->type())) return extent;

  auto elementType = traits.remove_cv(traits.remove_all_extents(field->type()));
  auto memoryLayout = control()->memoryLayout();

  const auto elementSize = memoryLayout->sizeOf(elementType);
  const auto totalSize = memoryLayout->sizeOf(field->type());

  if (!elementSize.has_value() || !totalSize.has_value()) return extent;
  if (!elementSize.value()) return extent;

  extent.stride = elementSize.value();
  extent.count = totalSize.value() / elementSize.value();
  return extent;
}

auto Binder::EmptySubobjects::componentsOf(bool isCompleteObject) const
    -> std::span<const EmptySubobjectComponent> {
  if (isCompleteObject) return components;
  return std::span(components).first(baseSubobjectComponentCount);
}

auto Binder::EmptySubobjects::lastOffset(bool isCompleteObject) const
    -> std::optional<std::uint64_t> {
  if (isCompleteObject) return lastInCompleteObject;
  return lastInBaseSubobject;
}

auto Binder::emptySubobjectComponent(ClassSymbol* classSymbol,
                                     bool isCompleteObject,
                                     std::uint64_t offset,
                                     ClassSubobjectExtent extent)
    -> std::optional<EmptySubobjectComponent> {
  if (!extent.count) return std::nullopt;

  const auto& subobjects = emptySubobjects(classSymbol);
  const auto last = subobjects.lastOffset(isCompleteObject);
  if (!last) return std::nullopt;

  EmptySubobjectComponent component;
  component.symbol = classSymbol->resolvedDefinition();
  component.subobjects = &subobjects;
  component.isCompleteObject = isCompleteObject;
  component.offset = offset;
  component.extent = extent;
  component.lastEmptySubobjectOffset =
      offset + (extent.count - 1) * extent.stride + *last;
  return component;
}

void Binder::forgetEmptySubobjects(ClassSymbol* classSymbol) {
  auto it = emptySubobjects_.find(classSymbol);
  if (it == emptySubobjects_.end()) return;
  it->second = EmptySubobjects{};
}

auto Binder::emptySubobjects(ClassSymbol* classSymbol)
    -> const EmptySubobjects& {
  classSymbol = classSymbol->resolvedDefinition();

  const auto layout = classSymbol->layout();

  if (auto it = emptySubobjects_.find(classSymbol);
      it != emptySubobjects_.end() && it->second.layout == layout) {
    return it->second;
  }

  EmptySubobjects entry;
  entry.layout = layout;

  if (layout) {
    entry.isEmptyClass = layout->isAbiEmpty();

    for (auto base : classSymbol->baseClasses()) {
      if (base->isVirtual()) continue;
      auto baseClass = resolved_base_class(base);
      if (!baseClass) continue;
      auto baseInfo = layout->getBaseInfo(baseClass);
      if (!baseInfo) continue;
      if (auto component = emptySubobjectComponent(
              baseClass, false, baseInfo->offset, ClassSubobjectExtent{}))
        entry.components.push_back(*component);
    }

    for (auto field : views::members(classSymbol) | views::non_static_fields) {
      auto fieldClass = fieldElementClass(field);
      if (!fieldClass || !fieldClass->layout()) continue;
      auto fieldInfo = layout->getFieldInfo(field);
      if (!fieldInfo) continue;
      if (auto component = emptySubobjectComponent(
              fieldClass, true, fieldInfo->offset, fieldArrayExtent(field)))
        entry.components.push_back(*component);
    }

    entry.baseSubobjectComponentCount = entry.components.size();

    for (auto virtualBase : layout->virtualBases()) {
      auto info = layout->getVirtualBaseInfo(virtualBase);
      if (!info) continue;
      if (auto component = emptySubobjectComponent(
              virtualBase, false, info->offset, ClassSubobjectExtent{}))
        entry.components.push_back(*component);
    }
  }

  if (entry.isEmptyClass) entry.lastInBaseSubobject = 0;
  for (const auto& component : entry.componentsOf(false)) {
    entry.lastInBaseSubobject = std::max(entry.lastInBaseSubobject.value_or(0),
                                         component.lastEmptySubobjectOffset);
  }

  entry.lastInCompleteObject = entry.lastInBaseSubobject;
  for (const auto& component : entry.componentsOf(true)) {
    entry.lastInCompleteObject =
        std::max(entry.lastInCompleteObject.value_or(0),
                 component.lastEmptySubobjectOffset);
  }

  entry.components.shrink_to_fit();

  auto& cached = emptySubobjects_[classSymbol];
  cached = std::move(entry);
  return cached;
}

auto Binder::BuildRecordLayout::elementDisplacement(
    const EmptySubobjectComponent& component, std::uint64_t displacement)
    -> std::optional<std::uint64_t> {
  if (displacement < component.offset) return std::nullopt;
  if (displacement > component.lastEmptySubobjectOffset) return std::nullopt;

  displacement -= component.offset;
  if (!component.extent.stride) return displacement;

  const auto index = displacement / component.extent.stride;
  if (index >= component.extent.count) return std::nullopt;
  return displacement - index * component.extent.stride;
}

auto Binder::BuildRecordLayout::hasEmptySubobjectAt(
    const EmptySubobjectComponent& placed, ClassSymbol* symbol,
    std::uint64_t address) -> bool {
  const auto displacement = elementDisplacement(placed, address);
  if (!displacement) return false;

  placedWalk.clear();
  placedWalk.emplace_back(&placed, *displacement);

  while (!placedWalk.empty()) {
    const auto [component, inner] = placedWalk.back();
    placedWalk.pop_back();

    const auto subobjects = component->subobjects;
    if (subobjects->isEmptyClass && component->symbol == symbol && !inner)
      return true;

    for (const auto& part :
         subobjects->componentsOf(component->isCompleteObject)) {
      if (auto partDisplacement = elementDisplacement(part, inner))
        placedWalk.emplace_back(&part, *partDisplacement);
    }
  }

  return false;
}

auto Binder::BuildRecordLayout::placedAt(ClassSymbol* symbol,
                                         std::uint64_t address) -> bool {
  const auto first = placedComponents.lower_bound(
      address - std::min(address, maxPlacedComponentSpan));
  const auto last = placedComponents.upper_bound(address);

  return std::any_of(first, last, [&](const auto& placed) {
    return hasEmptySubobjectAt(placed.second, symbol, address);
  });
}

auto Binder::BuildRecordLayout::conflictsAt(
    const EmptySubobjectComponent& candidate, std::uint64_t offset) -> bool {
  if (placedComponents.empty()) return false;

  candidateWalk.clear();
  candidateWalk.emplace_back(&candidate, offset);

  while (!candidateWalk.empty()) {
    const auto [component, enclosingAddress] = candidateWalk.back();
    candidateWalk.pop_back();

    const auto subobjects = component->subobjects;
    const auto parts = subobjects->componentsOf(component->isCompleteObject);

    for (std::uint64_t index = 0; index != component->extent.count; ++index) {
      const auto address = enclosingAddress + component->offset +
                           index * component->extent.stride;
      if (address > maxPlacedSubobjectOffset) break;
      if (subobjects->isEmptyClass && placedAt(component->symbol, address))
        return true;
      for (const auto& part : parts) candidateWalk.emplace_back(&part, address);
    }
  }

  return false;
}

auto Binder::BuildRecordLayout::conflictsAt(
    const std::vector<EmptySubobjectComponent>& candidates,
    std::uint64_t offset) -> bool {
  return std::ranges::any_of(candidates, [&](const auto& candidate) {
    return conflictsAt(candidate, offset);
  });
}

auto Binder::BuildRecordLayout::classSubobjectOffset(
    const std::vector<EmptySubobjectComponent>& candidates, bool tryZero,
    std::uint64_t alignment) -> std::uint64_t {
  if (tryZero && !conflictsAt(candidates, 0)) return 0;

  auto offset = align_to(calculatedSize, alignment);
  while (conflictsAt(candidates, offset)) offset += alignment;
  return offset;
}

void Binder::BuildRecordLayout::appendCandidate(
    std::vector<EmptySubobjectComponent>& candidates, ClassSymbol* symbol,
    bool isCompleteObject, std::uint64_t offset, ClassSubobjectExtent extent) {
  if (auto component = binder.emptySubobjectComponent(symbol, isCompleteObject,
                                                      offset, extent))
    candidates.push_back(*component);
}

void Binder::BuildRecordLayout::recordClassSubobject(
    ClassSymbol* symbol, bool isCompleteObject, std::uint64_t offset,
    ClassSubobjectExtent extent) {
  const auto component =
      binder.emptySubobjectComponent(symbol, isCompleteObject, offset, extent);
  if (!component) return;

  maxPlacedSubobjectOffset =
      std::max(maxPlacedSubobjectOffset, component->lastEmptySubobjectOffset);
  maxPlacedComponentSpan = std::max(
      maxPlacedComponentSpan, component->lastEmptySubobjectOffset - offset);
  placedComponents.emplace(offset, *component);
}

void Binder::BuildRecordLayout::growSizeof(std::uint64_t offset,
                                           std::uint64_t sizeInBytes) {
  runningSizeof = std::max(runningSizeof, offset + sizeInBytes);
}

void Binder::BuildRecordLayout::recordEmptyComponent(
    std::uint64_t offset, std::uint64_t sizeInBytes) {
  growSizeof(offset, sizeInBytes);
  emptyComponentEnd = std::max(emptyComponentEnd, offset + sizeInBytes);
}

auto Binder::BuildRecordLayout::allocateBaseSubobject(ClassSymbol* base,
                                                      bool isVirtual)
    -> ClassLayout::MemberInfo {
  auto baseLayout = base->layout();

  auto baseAlignment = static_cast<int>(base->alignment());
  if (baseLayout && !baseLayout->virtualBases().empty()) {
    baseAlignment = static_cast<int>(baseLayout->nonVirtualAlignment());
  }
  baseAlignment = packAlignment(baseAlignment);

  const auto baseSizeInBytes = static_cast<int>(baseNonVirtualSize(base));
  const bool isEmpty = baseLayout && baseLayout->isAbiEmpty();

  const auto ownedPrimaries = indirectPrimariesAllocatedWith({base, isVirtual});

  std::vector<EmptySubobjectComponent> candidates;
  appendCandidate(candidates, base, false, 0);
  for (const auto& [primary, offset] : ownedPrimaries)
    appendCandidate(candidates, primary, false, offset);

  const auto baseOffset =
      classSubobjectOffset(candidates, isEmpty, std::max(baseAlignment, 1));

  if (!isEmpty) padTo(baseOffset);

  ClassLayout::MemberInfo baseInfo;
  baseInfo.offset = baseOffset;
  baseInfo.index = currentIndex++;
  if (isVirtual)
    layout->setVirtualBaseInfo(base, baseInfo);
  else
    layout->setBaseInfo(base, baseInfo);

  if (isEmpty) {
    const auto emptySize = memoryLayout->sizeOf(base->type()).value_or(1);
    if (isVirtual) {
      growSizeof(baseOffset, emptySize);
    } else {
      recordEmptyComponent(baseOffset, emptySize);
    }
  } else {
    calculatedSize = std::max(calculatedSize,
                              static_cast<int>(baseOffset) + baseSizeInBytes);
    emittedEnd = std::max(emittedEnd, baseOffset + baseSizeInBytes);
  }

  recordClassSubobject(base, false, baseOffset);
  for (const auto& [primary, offset] : ownedPrimaries)
    recordIndirectPrimaryPlacement(primary,
                                   {baseOffset + offset, baseInfo.index});
  calculatedAlignment = std::max(calculatedAlignment, baseAlignment);

  return baseInfo;
}

void Binder::BuildRecordLayout::layoutBases() {
  if (classSymbol->isUnion()) return;

  ClassSymbol* primaryBase = nullptr;
  if (!layout->primaryBaseIsVirtual()) primaryBase = layout->primaryBase();

  std::vector<ClassSymbol*> orderedBases;
  for (auto base : classSymbol->baseClasses()) {
    if (base->isVirtual()) continue;
    auto baseClassSymbol = resolved_base_class(base);
    if (!baseClassSymbol) continue;
    if (baseClassSymbol == primaryBase) {
      orderedBases.insert(orderedBases.begin(), baseClassSymbol);
    } else {
      orderedBases.push_back(baseClassSymbol);
    }
  }

  for (auto baseClassSymbol : orderedBases) {
    const auto baseInfo = allocateBaseSubobject(baseClassSymbol, false);

    if (baseClassSymbol == primaryBase) layout->setVtableIndex(baseInfo.index);
  }

  nextBitPos = static_cast<std::uint64_t>(calculatedSize) * 8;
}

void Binder::BuildRecordLayout::layoutVirtualBases() {
  if (classSymbol->isUnion()) return;

  const auto virtualBases = virtualBasesInInheritanceGraphOrder(classSymbol);

  for (auto virtualBase : virtualBases) {
    if (indirectPrimaryPlacement(virtualBase)) continue;
    if (isIndirectPrimary(virtualBase)) continue;
    std::ignore = allocateBaseSubobject(virtualBase, true);
  }

  for (auto virtualBase : virtualBases) {
    if (auto placement = indirectPrimaryPlacement(virtualBase))
      layout->setVirtualBaseInfo(virtualBase, *placement);
    else if (!layout->getVirtualBaseInfo(virtualBase))
      std::ignore = allocateBaseSubobject(virtualBase, true);
    layout->addVirtualBase(virtualBase);
  }

  nextBitPos = static_cast<std::uint64_t>(calculatedSize) * 8;
}

void Binder::BuildRecordLayout::recordIndirectPrimaryPlacement(
    ClassSymbol* primary, ClassLayout::MemberInfo info) {
  if (indirectPrimaryPlacement(primary)) return;
  indirectPrimaryPlacements.emplace_back(primary, info);
  recordClassSubobject(primary, false, info.offset);
}

auto Binder::BuildRecordLayout::indirectPrimaryPlacement(
    ClassSymbol* primary) const -> std::optional<ClassLayout::MemberInfo> {
  for (const auto& [base, info] : indirectPrimaryPlacements) {
    if (base == primary) return info;
  }
  return std::nullopt;
}

auto Binder::BuildRecordLayout::isIndirectPrimary(ClassSymbol* base) const
    -> bool {
  return std::ranges::contains(indirectPrimaryOwners, base,
                               [](const auto& entry) { return entry.first; });
}

void Binder::BuildRecordLayout::assignIndirectPrimaryOwner(
    ClassSymbol* cls, AllocatedBase owner) {
  auto classLayout = cls->layout();
  if (!classLayout || !classLayout->primaryBaseIsVirtual()) return;
  auto primary = classLayout->primaryBase();
  if (isIndirectPrimary(primary)) return;
  indirectPrimaryOwners.emplace_back(primary, owner);
}

void Binder::BuildRecordLayout::assignIndirectPrimaryOwners() {
  struct InheritanceFrame {
    ClassSymbol* classSymbol;
    AllocatedBase owner;
    std::size_t nextBase = 0;
  };

  const AllocatedBase self{classSymbol, false};
  if (layout->primaryBaseIsVirtual())
    indirectPrimaryOwners.emplace_back(layout->primaryBase(), self);

  std::vector<ClassSymbol*> virtualBases;
  std::vector<InheritanceFrame> frames{{classSymbol, self}};
  while (!frames.empty()) {
    auto& frame = frames.back();
    auto& bases = frame.classSymbol->baseClasses();
    if (frame.nextBase == bases.size()) {
      frames.pop_back();
      continue;
    }
    const auto isDirect = frames.size() == 1;
    auto owner = frame.owner;
    auto base = bases[frame.nextBase++];
    auto baseClass = resolved_base_class(base);
    if (!baseClass) continue;
    if (base->isVirtual()) {
      if (std::ranges::contains(virtualBases, baseClass)) continue;
      virtualBases.push_back(baseClass);
      owner = {baseClass, true};
    } else if (isDirect) {
      owner = {baseClass, false};
    }
    if (is_laid_out_without_virtual_base_subobjects(baseClass)) continue;
    assignIndirectPrimaryOwner(baseClass, owner);
    frames.push_back({baseClass, owner});
  }

  if (!layout->primaryBaseIsVirtual()) return;
  auto primary = layout->primaryBase();
  auto info = layout->getVirtualBaseInfo(primary);
  if (!info) return;
  recordIndirectPrimaryPlacement(primary, *info);
  IndirectPrimaryOffsets chain;
  appendPrimaryChain(chain, primary, {primary, true}, info->offset);
  for (const auto& [chained, offset] : chain)
    recordIndirectPrimaryPlacement(chained, {offset, info->index});
}

auto Binder::BuildRecordLayout::ownsIndirectPrimary(AllocatedBase owner,
                                                    ClassSymbol* primary) const
    -> bool {
  return std::ranges::contains(indirectPrimaryOwners,
                               std::pair{primary, owner});
}

void Binder::BuildRecordLayout::appendPrimaryChain(
    IndirectPrimaryOffsets& primaries, ClassSymbol* cls, AllocatedBase owner,
    std::uint64_t offset) const {
  while (cls) {
    auto classLayout = cls->layout();
    if (!classLayout || !classLayout->primaryBase()) return;
    auto primary = classLayout->primaryBase();
    auto info =
        classLayout->getBaseInfo(primary, classLayout->primaryBaseIsVirtual());
    if (!info) return;
    const auto primaryOffset = offset + info->offset;
    if (classLayout->primaryBaseIsVirtual()) {
      if (!ownsIndirectPrimary(owner, primary)) return;
      if (!std::ranges::contains(primaries, primary,
                                 [](const auto& entry) { return entry.first; }))
        primaries.emplace_back(primary, primaryOffset);
      owner = {primary, true};
    }
    cls = primary;
    offset = primaryOffset;
  }
}

auto Binder::BuildRecordLayout::indirectPrimariesAllocatedWith(
    AllocatedBase root) const -> IndirectPrimaryOffsets {
  struct PlacementWork {
    ClassSymbol* classSymbol;
    std::uint64_t offset;
  };

  IndirectPrimaryOffsets primaries;
  std::vector<PlacementWork> pending{{root.symbol, 0}};
  while (!pending.empty()) {
    auto [cls, offset] = pending.back();
    pending.pop_back();
    auto classLayout = cls->layout();
    if (!classLayout) continue;
    if (is_laid_out_without_virtual_base_subobjects(cls)) continue;
    appendPrimaryChain(primaries, cls, root, offset);
    for (auto base : cls->baseClasses() | std::views::reverse) {
      if (base->isVirtual()) continue;
      auto baseClass = resolved_base_class(base);
      if (!baseClass) continue;
      if (auto info = classLayout->getBaseInfo(baseClass))
        pending.push_back({baseClass, offset + info->offset});
    }
  }
  return primaries;
}

void Binder::BuildRecordLayout::closeBitfieldRun() {
  if (!inBitfieldRun) return;

  calculatedSize = (nextBitPos + 7) / 8;

  auto allocUnitSizeBytes =
      static_cast<std::uint32_t>(calculatedSize - runStartByte);

  for (auto f : runFields) {
    if (auto info = layout->getFieldInfo(f)) {
      auto updated = *info;
      updated.allocUnitSizeBytes = allocUnitSizeBytes;
      layout->setFieldInfo(f, updated);
    }
  }

  runFields.clear();
  inBitfieldRun = false;
  emittedEnd = std::max(emittedEnd, static_cast<std::uint64_t>(calculatedSize));
  currentIndex++;
}

auto Binder::BuildRecordLayout::layoutBitfield(FieldSymbol* field)
    -> std::expected<bool, std::string> {
  const bool isUnion = classSymbol->isUnion();

  int bitWidth = 0;
  if (auto& bfw = field->bitFieldWidth()) {
    if (auto iv = std::get_if<ConstInt>(&*bfw)) {
      bitWidth = static_cast<int>(iv->toIntMax());
    }
  }

  if (bitWidth == 0) {
    layoutZeroWidthBitfield(field);
    return true;
  }

  auto fieldAlign = field->effectiveAlignment();
  auto fieldSizeBytes =
      static_cast<int>(memoryLayout->sizeOf(field->type()).value_or(0));
  auto fieldSizeBits = fieldSizeBytes * 8;

  const bool hasExcessWidth = fieldSizeBits > 0 && bitWidth > fieldSizeBits;
  auto valueBits = bitWidth;
  if (hasExcessWidth) {
    fieldAlign = static_cast<int>(
        memoryLayout->alignmentOf(excessWidthContainer(bitWidth)).value_or(1));
    valueBits = fieldSizeBits;
  }

  auto aggregateAlign = fieldAlign;
  if (!field->name() && !hasExcessWidth &&
      !memoryLayout->unnamedBitFieldAlignsAggregate())
    aggregateAlign = 1;

  if (isUnion) {
    field->setLocalOffset(0);
    field->setBitFieldOffset(0);

    ClassLayout::MemberInfo fieldInfo;
    fieldInfo.offset = 0;
    fieldInfo.index = 0;
    fieldInfo.bitOffset = 0;
    fieldInfo.bitWidth = valueBits;
    fieldInfo.allocUnitSizeBytes = (bitWidth + 7) / 8;
    layout->setFieldInfo(field, fieldInfo);

    auto fieldSizeForUnion = (bitWidth + 7) / 8;
    calculatedSize = std::max(calculatedSize, fieldSizeForUnion);
    calculatedAlignment = std::max(calculatedAlignment, aggregateAlign);
    return true;
  }

  if (hasExcessWidth || field->explicitAlignment()) {
    closeBitfieldRun();
    nextBitPos = align_to(nextBitPos, fieldAlign * 8);
    calculatedSize = nextBitPos / 8;
  } else if (fieldSizeBits > 0 && keepsBitFieldInAllocationUnit(field)) {
    auto startUnit = nextBitPos / fieldSizeBits;
    auto endUnit = (nextBitPos + bitWidth - 1) / fieldSizeBits;
    if (startUnit != endUnit) {
      if (inBitfieldRun) {
        closeBitfieldRun();
      }
      nextBitPos = align_to(nextBitPos, fieldSizeBits);
      calculatedSize = nextBitPos / 8;
    }
  }

  if (!inBitfieldRun) {
    runStartByte = calculatedSize;
    nextBitPos = static_cast<std::uint64_t>(runStartByte) * 8;
    padTo(static_cast<std::uint64_t>(runStartByte));
    runIndex = currentIndex;
    inBitfieldRun = true;
    runFields.clear();
  }

  auto bitOffsetInRun =
      nextBitPos - static_cast<std::uint64_t>(runStartByte) * 8;

  field->setLocalOffset(runStartByte);
  field->setBitFieldOffset(bitOffsetInRun);

  ClassLayout::MemberInfo fieldInfo;
  fieldInfo.offset = runStartByte;
  fieldInfo.index = runIndex;
  fieldInfo.bitOffset = bitOffsetInRun;
  fieldInfo.bitWidth = valueBits;
  layout->setFieldInfo(field, fieldInfo);

  runFields.push_back(field);
  nextBitPos += bitWidth;
  calculatedAlignment = std::max(calculatedAlignment, aggregateAlign);

  return true;
}

auto Binder::BuildRecordLayout::excessWidthContainer(int bitWidth) const
    -> const Type* {
  auto control = binder.control();
  const Type* candidates[] = {
      control->getSignedCharType(),  control->getShortIntType(),
      control->getIntType(),         control->getLongIntType(),
      control->getLongLongIntType(), control->getInt128Type()};
  const Type* container = nullptr;
  std::size_t containerSize = 0;
  for (auto candidate : candidates) {
    auto size = memoryLayout->sizeOf(candidate);
    if (!size || *size * 8 > static_cast<std::size_t>(bitWidth)) continue;
    if (*size <= containerSize) continue;
    container = candidate;
    containerSize = *size;
  }
  return container;
}

void Binder::BuildRecordLayout::layoutZeroWidthBitfield(FieldSymbol* field) {
  if (inBitfieldRun) closeBitfieldRun();

  if (memoryLayout->unnamedBitFieldAlignsAggregate())
    calculatedAlignment = std::max(calculatedAlignment, field->alignment());

  if (classSymbol->isUnion()) return;
  if (!memoryLayout->sizeOf(field->type()).value_or(0)) return;

  nextBitPos = align_to(nextBitPos, field->alignment() * 8);
  calculatedSize = (nextBitPos + 7) / 8;
}

auto Binder::BuildRecordLayout::layoutRegularField(FieldSymbol* field)
    -> std::expected<bool, std::string> {
  const bool isUnion = classSymbol->isUnion();

  closeBitfieldRun();

  const bool isEmptyDataMember = binder.traits.is_zero_size_subobject(field);

  std::optional<std::size_t> size;
  if (binder.traits.is_unbounded_array(field->type())) {
    size = 0;
  } else if (isEmptyDataMember) {
    size = 0;
  } else {
    size = memoryLayout->sizeOf(field->type());
  }

  if (!size.has_value()) {
    return std::unexpected(
        std::format("size of incomplete type '{}'",
                    to_diag_string(field->type(), field->name())));
  }

  const auto fieldOffset = regularFieldOffset(field, isEmptyDataMember);
  field->setLocalOffset(static_cast<int>(fieldOffset));

  if (!isEmptyDataMember) padTo(fieldOffset);

  ClassLayout::MemberInfo fieldInfo;
  fieldInfo.offset = fieldOffset;
  fieldInfo.index = isUnion ? 0 : currentIndex++;
  layout->setFieldInfo(field, fieldInfo);

  if (!isEmptyDataMember) {
    auto allocatedExtent = static_cast<std::uint64_t>(size.value());
    auto emittedExtent = allocatedExtent;

    if (field->isNoUniqueAddress() &&
        unqualified_cast<ClassType>(field->type())) {
      emittedExtent = binder.traits.non_virtual_size(field->type());
      allocatedExtent =
          std::max(emittedExtent, binder.traits.data_size(field->type()));
    }

    calculatedSize = std::max(calculatedSize,
                              static_cast<int>(fieldOffset + allocatedExtent));
    if (!isUnion)
      emittedEnd = std::max(emittedEnd, fieldOffset + emittedExtent);
  }

  if (field->isNoUniqueAddress()) {
    const auto memberSize = memoryLayout->sizeOf(field->type()).value_or(0);
    if (isEmptyDataMember) {
      recordEmptyComponent(fieldOffset, memberSize);
    } else {
      growSizeof(fieldOffset, memberSize);
    }
  }

  if (auto elementClass = binder.fieldElementClass(field)) {
    recordClassSubobject(elementClass, true, fieldOffset,
                         binder.fieldArrayExtent(field));
  }

  nextBitPos = static_cast<std::uint64_t>(calculatedSize) * 8;

  calculatedAlignment =
      std::max(calculatedAlignment, field->effectiveAlignment());
  return true;
}

auto Binder::BuildRecordLayout::regularFieldOffset(FieldSymbol* field,
                                                   bool isEmptyDataMember)
    -> std::uint64_t {
  if (classSymbol->isUnion()) return 0;
  auto fieldAlign = field->effectiveAlignment();
  if (auto elementClass = binder.fieldElementClass(field)) {
    std::vector<EmptySubobjectComponent> candidates;
    appendCandidate(candidates, elementClass, true, 0,
                    binder.fieldArrayExtent(field));
    return classSubobjectOffset(candidates, isEmptyDataMember,
                                std::max(fieldAlign, 1));
  }
  return static_cast<std::uint64_t>(align_to(calculatedSize, fieldAlign));
}

auto Binder::BuildRecordLayout::isPackedClass() const -> bool {
  return findAttribute(classSymbol->attributes(), AttributeKind::kPacked) !=
         nullptr;
}

auto Binder::BuildRecordLayout::packAlignment(int alignment) const -> int {
  if (packValue <= 0) return alignment;
  return std::min(alignment, packValue);
}

auto Binder::BuildRecordLayout::keepsBitFieldInAllocationUnit(
    FieldSymbol* field) const -> bool {
  if (packValue > 0) return false;
  return !field->isPacked();
}

auto Binder::BuildRecordLayout::layoutFields()
    -> std::expected<bool, std::string> {
  FieldSymbol* lastField = nullptr;

  for (auto field : views::members(classSymbol) | views::non_static_fields) {
    if (lastField && binder.traits.is_unbounded_array(lastField->type())) {
      return std::unexpected(
          std::format("size of incomplete type '{}'",
                      to_diag_string(lastField->type(), lastField->name())));
    }

    if (!field->alignment()) {
      if (isDependent(binder.unit_, field->type())) return false;
      if (binder.traits.is_variable_length_array(field->type())) return false;
      return std::unexpected(
          std::format("alignment of incomplete type '{}'",
                      to_diag_string(field->type(), field->name())));
    }

    if (field->isBitField()) {
      if (auto status = layoutBitfield(field); !status) return status;
    } else {
      if (auto status = layoutRegularField(field); !status) return status;
    }

    lastField = field;
  }

  closeBitfieldRun();
  return true;
}

void Binder::BuildRecordLayout::propagateBaseFields() {
  for (auto base : classSymbol->baseClasses()) {
    auto baseClassSymbol = resolved_base_class(base);
    if (!baseClassSymbol) continue;

    auto baseLayout = baseClassSymbol->layout();
    if (!baseLayout) continue;

    auto baseInfo = layout->getBaseInfo(baseClassSymbol, base->isVirtual());
    if (!baseInfo) continue;

    copyFieldInfos(baseLayout, baseInfo->offset, baseInfo->index);
  }

  auto virtualBases = virtual_base_initialization_order(classSymbol);
  for (auto base : virtualBases | std::views::reverse) {
    auto info = layout->getVirtualBaseInfo(base);
    if (!info || !base->layout()) continue;
    copyFieldInfos(base->layout(), info->offset, info->index);
  }

  propagateAnonymousFields();
}

void Binder::BuildRecordLayout::propagateAnonymousFields() {
  for (auto field : views::members(classSymbol) | views::non_static_fields) {
    auto anonymous = anonymous_member_class(field);
    if (!anonymous) continue;
    auto anonymousLayout = anonymous->layout();
    auto fieldInfo = layout->getFieldInfo(field);
    if (!anonymousLayout || !fieldInfo) continue;
    copyFieldInfos(anonymousLayout, fieldInfo->offset, fieldInfo->index);
  }
}

void Binder::BuildRecordLayout::copyFieldInfos(const ClassLayout* ownerLayout,
                                               std::uint64_t ownerOffset,
                                               std::uint32_t containingIndex) {
  for (auto [field, info] : ownerLayout->sortedFieldInfos()) {
    info.offset += ownerOffset;
    info.index = containingIndex;
    layout->setFieldInfo(field, info);
  }
}

void Binder::BuildRecordLayout::padTo(std::uint64_t offset) {
  if (offset <= emittedEnd) return;
  layout->addPadding(currentIndex++, emittedEnd, offset - emittedEnd);
  emittedEnd = offset;
}

void Binder::BuildRecordLayout::finalize() {
  if (auto requested = classSymbol->explicitAlignment()) {
    if (requested < calculatedAlignment) {
      binder.error(classSymbol->location(),
                   std::format("requested alignment is less than minimum "
                               "alignment of {} for type '{}'",
                               calculatedAlignment,
                               to_diag_string(classSymbol->type())));
    } else {
      calculatedAlignment = requested;
    }
  }

  calculatedAlignment =
      std::max(calculatedAlignment, classSymbol->minimumAlignment());

  const auto dataSize = static_cast<std::uint64_t>(calculatedSize);

  calculatedSize = std::max(calculatedSize, static_cast<int>(runningSizeof));

  if (calculatedSize == 0 && !binder.isC()) calculatedSize = 1;

  calculatedSize = align_to(calculatedSize, calculatedAlignment);

  padTo(static_cast<std::uint64_t>(calculatedSize));

  classSymbol->setAlignment(calculatedAlignment);
  classSymbol->setSizeInBytes(calculatedSize);

  layout->setSize(calculatedSize);
  layout->setAlignment(calculatedAlignment);
  layout->setDataSize(dataSize);

  binder.forgetEmptySubobjects(classSymbol->resolvedDefinition());
  classSymbol->setLayout(std::move(layout));

  buildVTableLayout();
}

void Binder::BuildRecordLayout::buildVTableLayout() {
  auto classLayout = classSymbol->layout();
  if (!classLayout || !classLayout->hasVtable()) return;
  binder.buildVTableLayout(classSymbol);
}
}  // namespace cxx
