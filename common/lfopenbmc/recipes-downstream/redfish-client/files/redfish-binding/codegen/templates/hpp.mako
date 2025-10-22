// @generated
#pragma once
% if len(cpp_def.include_generated_headers) > 0:

% for header in sorted(cpp_def.include_generated_headers):
#include "${header}.hpp"
% endfor
% endif
% if isinstance(cpp_def, cpp.CppObjectDef) or isinstance(cpp_def, cpp.CppEnumDef):

#include "common.hpp"
% endif
% if len(cpp_def.include_headers) > 0:

% for header in sorted(cpp_def.include_headers):
#include <${header}>
% endfor
% endif

#include <exception>
#include <expected>

#include <nlohmann/json.hpp>

namespace redfish_binding
{
% if len(forward_declarations) > 0:

% for namespace, class_names in sorted(forward_declarations.items()):
namespace ${namespace}
{
% for class_name in sorted(class_names):
class ${class_name};
% endfor
} // namespace ${namespace}
% endfor
% endif

namespace ${cpp_def.identifier.namespace}
{

% if isinstance(cpp_def, cpp.CppTypeAliasDef):
using ${cpp_def.identifier.id} = ${cpp_def.type_alias.get_name(cpp_def.identifier.namespace)};
% elif isinstance(cpp_def, cpp.CppObjectDef):
class ${cpp_def.identifier.id} : public ResourceBaseWithError
{
  public:
    % for property in cpp_def.properties:
    Property<${property.cpp_type.get_name(cpp_def.identifier.namespace)}>& get${property.name_as_member}()
    {
      return ${property.name_as_member}_;
    }

    % endfor
  protected:
    IProperty* findProperty(const std::string& key) override
    {
      % for property in cpp_def.properties:
      if (key == ${property.name_as_member}_.key())
      {
        return &${property.name_as_member}_;
      }
      % endfor
      return ResourceBaseWithError::findProperty(key);
    }

    void forEachProperty(
        const std::function<void(const IProperty*)>& fn) const override
    {
      % for property in cpp_def.properties:
      fn(&${property.name_as_member}_);
      % endfor
      ResourceBaseWithError::forEachProperty(fn);
    }


  % if len(cpp_def.properties) > 0:
  private:
    % for property in cpp_def.properties:
    Property<${property.cpp_type.get_name(cpp_def.identifier.namespace)}> ${property.name_as_member}_{"${property.name}"};
    % endfor
  % endif
};
% elif isinstance(cpp_def, cpp.CppEnumDef):
enum class ${cpp_def.identifier.id}
{
  % for enum_key in cpp_def.enum:
  ${enum_key},
  % endfor
};

NLOHMANN_JSON_SERIALIZE_ENUM(${cpp_def.identifier.id}, {
  % for enum_key, enum_value in cpp_def.enum.items():
  {${cpp_def.identifier.id}::${enum_key},"${enum_value}"},
  % endfor
})
% endif

inline ${cpp_def.identifier.id} parse${cpp_def.identifier.id}(const std::string& json)
{
  return nlohmann::json::parse(json).template get<${cpp_def.identifier.id}>();
}

inline std::expected<${cpp_def.identifier.id}, std::string> tryParse${cpp_def.identifier.id}(const std::string& json)
{
  try
    {
        return parse${cpp_def.identifier.id}(json);
    }
    catch (const std::exception& exn)
    {
        return std::unexpected(exn.what());
    }
}

} // namespace ${cpp_def.identifier.namespace}

} // namespace redfish_binding
