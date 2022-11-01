"""
A lightweight implementation of JSON Schema for Python

This is expected to provide minimal required support
for JSON schema and a stop-gap measure while we evaluate
jsonschema. Hence the usage mode is purposefully set
similar.
"""


class ValidationError(Exception):
    def __init__(self, entry, error):
        self.entry = entry
        self.error = error
        super(ValidationError, self).__init__(entry + " : " + error)


def _integer_validate(data, schema, entry):
    if not isinstance(data, int):
        raise ValidationError(entry, "Not integer")
    if "minimum" in schema and data < schema["minimum"]:
        raise ValidationError(entry, "%d < %d" % (data, schema["minimum"]))
    if "maximum" in schema and data > schema["maximum"]:
        raise ValidationError(entry, "%d > %d" % (data, schema["maximum"]))
    if "enum" in schema and data not in schema["enum"]:
        raise ValidationError(entry, "%d not in %s" % (data, str(schema["enum"])))


def _string_validate(data, schema, entry):
    if not isinstance(data, str):
        raise ValidationError(entry, "Not string")


def _bool_validate(data, schema, entry):
    if not isinstance(data, bool):
        raise ValidationError(entry, "Not bool")


def _tuple_validate(data, schema, entry):
    def entry_idx(idx):
        return entry + "[" + str(idx) + "]"

    idx = 0
    for item in schema["items"]:
        if idx >= len(data):
            break
        validate(data[idx], item, entry_idx(idx))
        idx += 1
    if "additionalItems" in schema:
        while idx < len(data):
            validate(data[idx], schema["additionalItems"], entry_idx(idx))
            idx += 1


def _array_validate(data, schema, entry):
    def entry_idx(idx):
        return entry + "[" + str(idx) + "]"

    uniqueItems = schema.get("uniqueItems", False)
    if uniqueItems and len(set(data)) != len(data):
        raise ValidationError(entry, "Non-unique items: " + str(data))
    if not isinstance(data, list) and not isinstance(data, tuple):
        raise ValidationError(entry, "Not an array")
    if "minItems" in schema and len(data) < schema["minItems"]:
        raise ValidationError(
            entry,
            "Array len: %d less than expected: %d" % (len(data), schema["minItems"]),
        )
    if "maxItems" in schema and len(data) > schema["maxItems"]:
        raise ValidationError(
            entry,
            "Array len: %d greater than expected: %d" % (len(data), schema["maxItems"]),
        )
    if isinstance(schema["items"], list):
        _tuple_validate(data, schema, entry)
    elif isinstance(schema["items"], dict):
        idx = 0
        for d in data:
            validate(d, schema["items"], entry_idx(idx))
            idx += 1
    else:
        raise ValidationError(
            entry, "Array Schema has items which is neither object or array"
        )


def _object_validate(data, schema, entry):
    if not isinstance(data, dict):
        raise ValidationError(entry, "Not a Dict/Object")
    additional_allowed = schema.get("additionalProperties", True)
    required = schema.get("required", [])
    for req in required:
        if req not in data:
            raise ValidationError(entry, "Missing key: %s" % (req))
    if not additional_allowed:
        if not (set(data.keys()) <= set(schema["properties"].keys())):
            print(data.keys(), ">", schema["properties"].keys())
            unexpected = set(data.keys()) - set(schema["properties"].keys())
            raise ValidationError(entry, "Unexpected keys: " + str(unexpected))
    for k, v in schema["properties"].items():
        if k not in data:
            continue
        validate(data[k], v, entry + "." + k)


def validate(data, schema, entry="data"):
    if schema["type"] == "object":
        _object_validate(data, schema, entry)
    elif schema["type"] == "array":
        _array_validate(data, schema, entry)
    elif schema["type"] == "integer":
        _integer_validate(data, schema, entry)
    elif schema["type"] == "string":
        _string_validate(data, schema, entry)
    elif schema["type"] == "boolean":
        _bool_validate(data, schema, entry)
    else:
        raise ValidationError(entry, "Unknown schema type: " + schema["type"])
