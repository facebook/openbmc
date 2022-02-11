RULES = {"/api/sys/server": {"POST": ["wedge400", "wedge400c"]}}

RULES_REGEXP = {
    # forbid experimental API
    r"^/redfish/v1/Systems/.*": {
        "GET": [],
        "POST": [],
        "DELETE": [],
        "PATCH": [],
        "PUT": [],
    }
}
