"""
Robust Python AST Parser for CodonWeb Schema Definitions.
Leverages Python's official PEG/AST parser to eliminate fragile manual line-scanning.
"""
import ast
from typing import List, Set, Optional, Any
from tools.codegen.py_codegen.models import ClassMeta, FieldMeta



def _eval_literal(node: ast.AST) -> Any:
    """Safely extracts constant or literal string/number/bool/list value from AST."""
    try:
        return ast.literal_eval(node)
    except Exception:
        return ast.unparse(node)


def _is_schema_base(base_node: ast.AST) -> bool:
    name = ast.unparse(base_node).strip()
    return name in ("BaseModel", "SchemaModel")


def _is_custom_model_type(type_name: str, known_classes: Set[str]) -> bool:
    if type_name in known_classes:
        return True
    if len(type_name) > 0 and type_name[0].isupper():
        if type_name not in ("List", "Optional", "Dict", "Tuple", "Set", "Ptr", "ByteSlice"):
            return True
    return False


def _populate_field_from_call(field: FieldMeta, call_node: ast.Call) -> None:
    func_name = ast.unparse(call_node.func).strip()
    is_sample_helper = func_name in ("ItemsField", "ModelField")

    # 1. Positional arguments (e.g. Field("default_val", ...) or Field(...))
    if call_node.args:
        first_arg = call_node.args[0]
        if isinstance(first_arg, ast.Constant) and first_arg.value == ...:
            field.required = True
        elif isinstance(first_arg, ast.Ellipsis):
            field.required = True
        elif is_sample_helper:
            # Type sample positional argument for Codon compiler type deduction
            field.required = True
        else:
            field.has_default = True
            field.required = False
            if isinstance(first_arg, ast.Constant) and isinstance(first_arg.value, str):
                field.default_val = f'"{first_arg.value}"'
            else:
                field.default_val = ast.unparse(first_arg)

    # 2. Keyword arguments
    for kw in call_node.keywords:
        k = kw.arg
        v_node = kw.value
        v_raw = _eval_literal(v_node)

        if k == "min_length":
            field.min_length = int(v_raw)
        elif k == "max_length":
            field.max_length = int(v_raw)
        elif k == "ge":
            if isinstance(v_raw, float) or (isinstance(v_raw, str) and "." in v_raw):
                field.has_ge_f = True
                field.ge_f = float(v_raw)
            else:
                field.has_ge = True
                field.ge = int(v_raw)
        elif k == "le":
            if isinstance(v_raw, float) or (isinstance(v_raw, str) and "." in v_raw):
                field.has_le_f = True
                field.le_f = float(v_raw)
            else:
                field.has_le = True
                field.le = int(v_raw)
        elif k == "gt":
            if isinstance(v_raw, float) or (isinstance(v_raw, str) and "." in v_raw):
                field.has_gt_f = True
                field.gt_f = float(v_raw)
            else:
                field.has_gt = True
                field.gt = int(v_raw)
        elif k == "lt":
            if isinstance(v_raw, float) or (isinstance(v_raw, str) and "." in v_raw):
                field.has_lt_f = True
                field.lt_f = float(v_raw)
            else:
                field.has_lt = True
                field.lt = int(v_raw)
        elif k == "format":
            field.format = str(v_raw)
        elif k == "pattern":
            field.pattern = str(v_raw)
        elif k == "description":
            field.description = str(v_raw)
        elif k == "alias":
            field.alias = str(v_raw)
        elif k == "strip_whitespace":
            field.strip_whitespace = bool(v_raw)
        elif k == "min_items":
            field.min_items = int(v_raw)
        elif k == "max_items":
            field.max_items = int(v_raw)
        elif k == "choices":
            if isinstance(v_raw, (list, tuple)):
                field.choices = [str(x) for x in v_raw]
        elif k == "required":
            field.required = bool(v_raw)
        elif k == "default":
            field.has_default = True
            field.required = False
            if isinstance(v_node, ast.Constant) and isinstance(v_node.value, str):
                field.default_val = f'"{v_node.value}"'
            else:
                field.default_val = ast.unparse(v_node)


def parse_schemas_from_source(source_text: str) -> List[ClassMeta]:
    """Parses Codon source code into strongly typed ClassMeta list using Python AST."""
    # Strip any previously generated section
    auto_marker = "# ==================== AUTO-GENERATED METHODS"
    m_idx = source_text.find(auto_marker)
    clean_src = source_text[:m_idx] if m_idx >= 0 else source_text

    try:
        tree = ast.parse(clean_src)
    except SyntaxError as e:
        # Fallback graceful handling
        print(f"[AST Error] Syntax error parsing schema source: {e}")
        return []

    classes: List[ClassMeta] = []
    known_classes: Set[str] = set()

    # Pass 1: Identify schema classes
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            if any(_is_schema_base(base) for base in node.bases):
                base_name = ast.unparse(node.bases[0])
                cls_meta = ClassMeta(name=node.name, base_name=base_name)
                doc = ast.get_docstring(node)
                if doc:
                    cls_meta.docstring = doc
                classes.append(cls_meta)
                known_classes.add(node.name)

    if not classes:
        return []

    # Map class name to ClassMeta
    class_map = {c.name: c for c in classes}

    # Pass 2: Extract fields and custom methods
    for node in tree.body:
        if not isinstance(node, ast.ClassDef) or node.name not in class_map:
            continue

        cls_meta = class_map[node.name]

        for item in node.body:
            # Annotated assignments: e.g. username: str = StrField(...)
            if isinstance(item, ast.AnnAssign) and isinstance(item.target, ast.Name):
                fname = item.target.id
                ftype = ast.unparse(item.annotation).strip()
                field = FieldMeta(name=fname, field_type=ftype)

                if item.value:
                    if isinstance(item.value, ast.Call):
                        _populate_field_from_call(field, item.value)
                    else:
                        # Direct default literal: e.g. age: int = 18 or bio: Optional[str] = None
                        field.has_default = True
                        field.required = False
                        field.default_val = ast.unparse(item.value)

                cls_meta.fields.append(field)

            # Custom methods: e.g. def custom_validate(self): ...
            elif isinstance(item, ast.FunctionDef):
                cls_meta.custom_methods.append(ast.unparse(item))

    # Pass 3: Resolve composite and nested types
    for cls_meta in classes:
        for f in cls_meta.fields:
            raw_t = f.field_type

            # Optional[T]
            if raw_t.startswith("Optional[") and raw_t.endswith("]"):
                f.is_optional = True
                f.inner_type = raw_t[9:-1].strip()
                if not f.has_default:
                    f.has_default = True
                    f.required = False
                    f.default_val = "None"
            else:
                f.inner_type = raw_t

            # List[T]
            if f.inner_type.startswith("List[") and f.inner_type.endswith("]"):
                f.is_list = True
                f.list_item_type = f.inner_type[5:-1].strip()
                if _is_custom_model_type(f.list_item_type, known_classes):
                    f.is_submodel = True
                    f.submodel_name = f.list_item_type
            # Nested SubModel
            elif _is_custom_model_type(f.inner_type, known_classes):
                f.is_submodel = True
                f.submodel_name = f.inner_type

    return classes
