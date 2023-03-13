import requests, string, random
from pprint import pprint as pp
from uuid import uuid4

schema_examples='''
http://redfish.dmtf.org/schemas/Processor.v1_1_0.json
http://redfish.dmtf.org/schemas/ComputerSystem.v1_4_0.json
http://redfish.dmtf.org/schemas/v1/ProcessorCollection.json#/definitions/ProcessorCollection
http://redfish.dmtf.org/schemas/ResourceBlock.v1_0_0.json
'''


def schema_get(root='', path=''):
    try:
        ipath = path.split('/')[1:]
        schema = root
        for p in ipath:
            schema = schema[p]
        return schema, root
    except:
        schema = requests.get(path).json()
        if '#' in path:
            ref = '#' + path.split('#')[-1]
        else:
            ref = schema.get('$ref', '')
        if ref:
            return schema_get(schema, ref)
        return {}, root


def randStr(n):
    alpanumeric = string.ascii_uppercase + string.digits
    return ''.join([random.choice(alpanumeric) for _ in xrange(n)])


def safe_input(prompt):
    inp = raw_input(prompt)
    try:
        return eval(inp)
    except:
        return inp


def read_properties(schema, root):
    if '$ref' in schema:
        print 'got ref: %s' % schema['$ref']
        follow = (raw_input('follow?(y/n)') == 'y')
        if follow:
            new_schema, new_root = schema_get(root, schema['$ref'])
            if new_schema:
                return read_properties(new_schema, new_root)
        else:
            return safe_input('not following, enter your input:')
    if 'properties' in schema:
        if not schema['properties']:
            pp(schema)
            return safe_input('manual input: ')
        print schema.get('description', 'no description')
        print 'required: ', schema.get('required', [])
        props = {}
        for item in schema['properties'].items():
            print 'Input relates to :', schema.get('description', 'no description')
            if item[0] == '@odata.type':
                prop = root.get('title', safe_input(
                    '@odata.type is %s, press enter' % root.get('title', 'not found. type one and')))
                print item[0], prop
            elif item[0] == '@odata.context':
                odatatype = root.get('title', safe_input(
                    '@odata.type is %s, press enter' % root.get('title', 'not found. type one and')))
                split = odatatype.split('.')
                prop = '/redfish/v1/$metadata{0}.{1}'.format(split[0], split[-1])
                print item[0], prop
            else:
                print item[0]
                prop = read_properties(item[1], root)
            if prop:
                props[item[0]] = prop
        return props
    if 'anyOf' in schema:
        pp(zip(xrange(len(schema['anyOf'])), schema['anyOf']))
        inp = safe_input('choose id of choice or press enter for null: ')
        if inp != '' and type(inp) is int:
            choice = schema['anyOf'][inp if inp<len(schema['anyOf']) else 0]
            if '$ref' in choice:
                return read_properties(*schema_get(root, choice['$ref']))
            else:
                return read_properties(choice, root)
        else:
            return safe_input('manual input:')
    if 'string' in str(schema['type']):
        pp(schema)
        return safe_input('your string input:')
    elif 'boolean' in str(schema['type']) or 'number' in str(schema['type']):
        pp(schema)
        return safe_input('your input:')
    elif 'object' in str(schema['type']):
        if 'properties' in schema:
            print schema.get('description', 'No description')
            print 'required: ', schema.get('required', [])
            return read_properties(schema['properties'], root)
        else:
            pp(schema)
            return safe_input('manual input: ')
    elif 'array' in str(schema['type']):
        pp(schema)
        num = safe_input('# of items:')
        num = 0 if not num else num
        vals = []
        for i in xrange(num):
            print 'item ', i
            vals.append(read_properties(schema['items'], root))
        return vals
    else:
        print 'catch all:'
        print schema
        return safe_input('your input: ')


if __name__=='__main__':
    print schema_examples
    schema_url=safe_input('enter a schema url to generate the json from (see examples above):')
    schema, root = schema_get(path=schema_url)
    template=read_properties(schema, root)
    print '\n\nTemplate:\n'
    pp(template)
