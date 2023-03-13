#!/usr/bin/env python2
#
# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md
'''
    This program generates a template file for the Redfish Interface
    Emulator.

    The program uses a "index.json" file in the current working directory to
    construct the template.
    
Usage:
     python codegen_template.py <class> <template directory>

'''
import sys
import os
import re
import time
import keyword
import json
from collections import OrderedDict

orig_path = 'default'

def write_program_header(outfile, base_program_name):
    """ Writes a program header """
    outfile.write('#!/usr/bin/env python3\n')
    outfile.write('# Copyright Notice:\n')
    outfile.write('# Copyright 2017-2019 DMTF. All rights reserved.\n')
    outfile.write('# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md\n')
    outfile.write('\n')
    outfile.write('# {0}.py\n'.format(base_program_name))
    outfile.write('\n')
    outfile.write('import copy\n')
    outfile.write('import strgen\n')
    outfile.write('from api_emulator.utils import replace_recurse\n')
    outfile.write('\n')
    return

"""
    The insert_wildcards() routine goes through the mockup recursively, locates
    the "@odata.id" keys, then replaces the collection
    members values in the path with their corresponding wildcard.

    The wildcards to use is specified the rf_collections dictionary.  This
    contains a simple list of the name of the collection and the wildcard to
    use for the singleton member.
"""
rf_collections = {
    "Chassis":    "{ch_id}" ,
    "Systems":    "{cs_id}" ,
    "Managers":   "{mgr_id}"
}

def insert_wildcards(c):
    """ Recursively go through dictionary and insert wildcards """
    # print("insert_wildcards c: ", c)

    for k, v in c.items():
        if isinstance(v, dict):
            insert_wildcards(c[k])
        elif isinstance(v, list):
            for index, item in enumerate(v):
                insert_wildcards(item)
        elif isinstance (v, str):
            # print("key/value : ", k, "; ", v)
            # print("c[k] : ", c[k])
            matched = False
            if k == "@odata.id":
                pathlist = c[k].split("/")
                #  print("odata.id found; pathlist: ", pathlist)
                for idx, item in enumerate(pathlist):
                    # print("item: ", item)
                    for wc_key, wc_value in rf_collections.items():
                        # print("collection:", wc_key)
                        if item == wc_key:
                            # print("Matched collection: ", item)
                            matched = True
                            break
                    if matched == True:
                        pathlist[idx+1] = wc_value
                        matched = False
                        # print("Replaced with: ", wc_value)
       
                # print("pathlist: ", pathlist)
                c[k] = "/".join(pathlist)

"""
    The write_template() routine uses the mockup file (index.json) to create the _TEMPLATE
    structure in the template source file.

        1. Read in the Redfish mockup, retain object order
        2. If "Id" property exists, replace its value with the "{id}" wildcard
        3. Call insert_wildcards() to recursive search the mockup of @odata.id and
            add wildcards into the path value.
        4. Write the resulting JSON into the template source file


"""
def write_template(outfile, base_program_name):
    outfile.write('_TEMPLATE = \\\n')
    global orig_path
    input_file = os.path.join(orig_path, 'index.json')
    exists = os.path.exists (input_file)
    infile = open(input_file, 'r')
    # Read in the Redfish mockup, retain object order
    data = json.load(infile, object_pairs_hook=OrderedDict)
    # If "Id" property exists, replace its value with the "{id}" wildcard
    data["Id"] = "{id}"
    insert_wildcards(data)
    json.dump(data, outfile, indent=4)
    outfile.write("\n\n")
    return

def write_program_end(outfile, base_program_name):
    ''' Write the get_*_instance() function '''
    argument_string = "def get_{0}_instance(wildcards):\n".format(base_program_name)
    outfile.write(argument_string)
    outfile.write('    c = copy.deepcopy(_TEMPLATE)\n')
    outfile.write('    replace_recurse(c, wildcards)\n')
    outfile.write('    return c\n')
    return

def write_program(outfile, base_program_name):
    """ Write the python program file """
    param_list = []
    write_program_header(outfile, base_program_name)
    write_template(outfile, base_program_name)
    write_program_end(outfile, base_program_name)

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
        program_name = '{0}.py'.format(base_program_name)

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
