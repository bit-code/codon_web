"""
Industrial-grade Schema AST Metadata Definitions for CodonWeb CodeGen.
Pure data contracts decoupling AST parsing from template emission.
"""
from dataclasses import dataclass, field
from typing import List, Optional, Dict, Any


@dataclass
class FieldMeta:
    name: str
    field_type: str
    has_default: bool = False
    default_val: str = ""
    required: bool = True
    min_length: int = 0
    max_length: int = 0
    has_ge: bool = False
    ge: int = 0
    has_le: bool = False
    le: int = 0
    has_gt: bool = False
    gt: int = 0
    has_lt: bool = False
    lt: int = 0
    has_ge_f: bool = False
    ge_f: float = 0.0
    has_le_f: bool = False
    le_f: float = 0.0
    has_gt_f: bool = False
    gt_f: float = 0.0
    has_lt_f: bool = False
    lt_f: float = 0.0
    format: str = ""
    pattern: str = ""
    choices: List[str] = field(default_factory=list)
    description: str = ""
    alias: str = ""
    strip_whitespace: bool = False
    min_items: int = 0
    max_items: int = 0
    is_list: bool = False
    list_item_type: str = ""
    is_optional: bool = False
    inner_type: str = ""
    is_submodel: bool = False
    submodel_name: str = ""

    @property
    def lookup_key(self) -> str:
        return self.alias if self.alias else self.name

    @property
    def is_int(self) -> bool:
        return self.field_type == "int"

    @property
    def is_float(self) -> bool:
        return self.field_type == "float"

    @property
    def is_bool(self) -> bool:
        return self.field_type == "bool"

    @property
    def is_str(self) -> bool:
        return not (self.is_int or self.is_float or self.is_bool or self.is_list or self.is_submodel)

    @property
    def fallback(self) -> str:
        if self.has_default:
            return self.default_val
        if self.is_int:
            return "0"
        if self.is_float:
            return "0.0"
        if self.is_bool:
            return "False"
        return '""'



@dataclass
class ClassMeta:
    name: str
    base_name: str = "BaseModel"
    docstring: str = ""
    fields: List[FieldMeta] = field(default_factory=list)
    custom_methods: List[str] = field(default_factory=list)

    @property
    def has_custom_validate(self) -> bool:
        return any("def custom_validate" in m for m in self.custom_methods)
