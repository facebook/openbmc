#!/usr/bin/env python2
#
# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md
'''
    This program generates a Flask-restful API file for the Redfish Interface
    Emulator.  It generates the code for both the singleton resource and the
    collection resource.
    
Usage:
     python codegen_api.py <class> <output_directory>

     The <class> parameter is the name of the singleton class (e.g. Manager).
     The value of <class> is used to construct the name of the output source
     file (e.g. <class>_api.py)

     The <output_directory) is the name of the directory in which to place the
     generated source file.

'''
import sys
import os
import re
import time
import keyword

orig_path = 'default'

def write_program_header(outfile, base_program_name):
    """ Writes a program header """
    outfile.write('# Copyright Notice:\n')
    outfile.write('# Copyright 2017-2019 DMTF. All rights reserved.\n')
    outfile.write('# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md\n')
    outfile.write('\n')
    outfile.write('# {0}.py\n'.format(base_program_name))
    outfile.write('\n')
    outfile.write('import g\n')
    outfile.write('import sys, traceback\n')
    outfile.write('import logging\n')
    outfile.write('import copy\n')
    outfile.write('from flask import Flask, request, make_response, render_template\n')
    outfile.write('from flask_restful import reqparse, Api, Resource\n')
    # TODO Auto-insert imports from subordinate resources
    outfile.write("\n")
    outfile.write("members = []\n")
    outfile.write("member_ids[]\n")
    outfile.write("foo = 'false'\n")
    outfile.write("INTERNAL_ERROR = 500\n")
    outfile.write("\n")
    return

def write_singleton_api(outfile, base_program_name):
    ''' Write the singleton resource API function '''
    argument_string = "class {0}(Resource):\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\t# INIT\n")
    outfile.write("\tdef __init__(self):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tglobal config\n")
    outfile.write("\t\t\tglobal wildcards\n")
    outfile.write("\t\t\twildcards = kwargs\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\n")
    
    outfile.write("\t# HTTP GET\n")
    outfile.write("\tdef get(self, ident):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tresp = 404\n")
    outfile.write("\t\t\tfor cfg in members:\n")
    outfile.write("\t\t\t\tif (ident == cfg[\"Id\"]):\n")
    outfile.write("\t\t\t\t\tconfig = cfg\n")
    outfile.write("\t\t\t\t\tresp = config, 200\n")
    outfile.write("\t\t\t\t\tbreak\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write("\n")

    outfile.write("\t# HTTP POST\n")
    outfile.write("\tdef post(self, ident):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tglobal config\n")
    outfile.write("\t\t\tglobal wildcards\n")
    outfile.write("\t\t\twildcards['id'] = ident\n")
    outfile.write("\t\t\tconfig=get_Chassis_instance(wildcards)\n")
    outfile.write("\t\t\tmembers.append(config)\n")
    outfile.write("\t\t\tmember_ids.append({'@odata.id': config['@odata.id']})\n")
    outfile.write("\t\t\tglobal foo\n")
    outfile.write("\t\t\tif  (foo == 'false'):\n")
    outfile.write("\t\t\t\t# TODO - add code for subordinate resources\n")
    outfile.write("\t\t\t\tfoo = 'true'\n")
    outfile.write("\t\t\tresp = config, 200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write("\n")

    outfile.write("\t# HTTP PATCH\n")
    outfile.write("\tdef patch(self, ident):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tfor cfg in members:\n")
    outfile.write("\t\t\t\tif (ident == cfg[\"Id\"]):\n")
    outfile.write("\t\t\t\t\tbreak\n")
    outfile.write("\t\t\tconfig = cfg\n")
    outfile.write("\t\t\tlogging.info(config)\n")
    outfile.write("\t\t\tfor key, value in raw_dict.items():\n")
    outfile.write("\t\t\t\tconfig[key] = value\n")
    outfile.write("\t\t\tresp = config, 200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write("\n")

    outfile.write("\t# HTTP DELETE\n")
    outfile.write("\tdef delete(self, ident):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tidx = 0\n")
    outfile.write("\t\t\tfor cfg in members:\n")
    outfile.write("\t\t\t\tif (ident == cfg[\"Id\"]):\n")
    outfile.write("\t\t\t\t\tbreak\n")
    outfile.write("\t\t\t\tidx += 1\n")
    outfile.write("\t\t\tmembers.pop(idx)\n")
    outfile.write("\t\t\tmember_ids.pop(idx)\n")
    outfile.write("\t\t\tresp = 200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write('\n\n')
    return

def write_collection_api(outfile, base_program_name):
    ''' Write the collection resource API function '''
    argument_string = "class {0}Collection(Resource):\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\tdef __init__(self):\n")
    outfile.write("\t\tself.rb = g.rest_base\n")
    outfile.write("\t\tself.config = {\n")
    argument_string = "\t\t\t'@odata.context': self.rb + '$metadata#{0}Collection.{0}Collection'\n".format(base_program_name)
    outfile.write(argument_string)
    argument_string = "\t\t\t'@odata.id': self.rb + '{0}Collection'\n".format(base_program_name)
    outfile.write(argument_string)
    argument_string = "\t\t\t'@odata.type': '#{0}Collection.1.0.0.{0}Collection'\n".format(base_program_name)
    outfile.write(argument_string)
    argument_string = "\t\t\t'Name': 'Chassis Collection'\n"
    outfile.write(argument_string)
    outfile.write("\t\t\t'Links': {}\n")
    outfile.write("\t\t}\n")
    outfile.write("\t\tself.config['Links']['Members@odata.count'] = len(member_ids)\n")
    outfile.write("\t\tself.config['Links']['Members'] = member_ids\n")
    outfile.write("\n")
    outfile.write("\tdef get(self):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tresp = self.config, 200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write("\n")

    outfile.write("\tdef post(self):\n")
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tg.api.add_resource(ChassisAPI, '/redfish/v1/Chassiss/<string:ident>')\n")
    outfile.write("\t\t\tresp=self.config,200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    outfile.write("\t\treturn resp\n")
    outfile.write('\n\n')
    return

def write_create_call(outfile, base_program_name):
    ''' Write the create_<class>() function '''
    argument_string = "class Create{0}(Resource):\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\tdef __init__(self, **kwargs):\n")
    argument_string = "\t\tlogging.info('CreateChassis init called')\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\t\tif 'resource_class_kwargs' in kwargs:\n")
    outfile.write("\t\t\tglobal wildcards\n")
    outfile.write("\t\t\twildcards = copy.deepcopy(kwargs['resource_class_kwargs'])\n")
    outfile.write("\t\t\tlogging.debug(wildcards, wildcards.keys())\n")
    outfile.write("\n")

    outfile.write("\tdef put(self,ident):\n")
    argument_string = "\t\tlogging.info('CreateChassis put called')\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\t\ttry:\n")
    outfile.write("\t\t\tglobal config\n")
    outfile.write("\t\t\tglobal wildcards\n")
    outfile.write("\t\t\twildcards['id'] = ident\n")
    argument_string = "\t\t\tconfig=get_Chassis_instance(wildcards)\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\t\t\tmembers.append(config)\n")
    outfile.write("\t\t\tmember_ids.append({'@odata.id': config['@odata.id']})\n")
    outfile.write("\t\t\t# TODO - add code for subordinate resources\n")
    outfile.write("\t\t\tresp = config, 200\n")
    outfile.write("\t\texcept Exception:\n")
    outfile.write("\t\t\ttraceback.print_exc()\n")
    outfile.write("\t\t\tresp = INTERNAL_ERROR\n")
    argument_string = "\t\tlogging.info('CreateChassis init exit')\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write("\t\treturn resp\n")
    return

def write_program(outfile, base_program_name):
    """ Write the python program file """
    write_program_header(outfile, base_program_name)
    write_singleton_api (outfile, base_program_name)
    write_collection_api(outfile, base_program_name)
    write_create_call   (outfile, base_program_name)

def file_exists(file_name):
    """ Check if file already exists. """
    exists = True
    try:
        with open(file_name) as f:
            pass
    except IOError as io_error:
        exists = False
    return exists

def create_folder_under_current_directory(folder_name):
    current_path = os.getcwd()
    global orig_path
    orig_path = current_path
    new_path = os.path.join(current_path, folder_name)
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)
    return new_path

# Start of main program.
def main(argv=None):
    if argv == None:
	    argv = sys.argv
    # Set the default return value to indicate success.
    status = 0
    # There must be at least one argument that is the program name.
    if len(argv) < 2:
        print ('Program: dynamic_codegen.py\nUse -h or --help for more information.')
        return status
    # Get the program name or  "-h" or "--help".
    base_program_name = argv[1]
    if base_program_name == '-h' or base_program_name == '--help':
        print (__doc__)
        return status
    program_name = base_program_name
    # If program name include "json", truncate it to the base_program_name.
    if base_program_name.endswith('.json'):
        base_program_name = base_program_name[:-3]
    try:
        # Create the name of the template.
        #   The name can be obtained: from the command line, extracted from the
        #   index.json file, or extracted from the filename (e.g. Chassis.json).
        #   For now, make it a command parameter
        #
        program_name = '{0}_api.py'.format(base_program_name)

        # If the output file already exists, then prompt the user to overwrite the file.
        if file_exists(program_name):
            print ("File '{0}' already exists. Enter 'y' or 'Y' to overwrite the file. >".format(program_name))
            c = raw_input()
            if c != 'y' and c != 'Y':
                return status
        # Create a 'base_program_name' folder.
        new_path = create_folder_under_current_directory("templates")
        # Change the current working directory to the new folder.
        os.chdir(new_path)
        
        # Open the program file and write the program.
        with open(program_name, 'w') as outfile:
            write_program(outfile, base_program_name)
            print ('Created program {0}'.format(program_name))
    except EnvironmentError as environment_error:
        print (environment_error)
        status = -1
    except ValueError as value_error:
        print (value_error)
        status = -1
    return status

if __name__ == "__main__":
    sys.exit(main())
