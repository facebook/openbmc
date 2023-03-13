#!/usr/bin/env python2
#
# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md
'''
    This program generates a dynamic template file for the Redfish Interface
    Emulator from a mockup file.
    
    The resultant template file needs to be modified to indicated the
    wildcards.

Usage:
     python dynamic_codegen.py <class name> <template directory>

'''
import sys
import os
import re
import time
import keyword

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

def write_template(outfile, base_program_name):
    """
    Write the _TEMPLATE dictionary declaration section of the file

    The mockup file, index.json, is read to determine the contents of the
    _TEMPLATE
    
    There are two of ways to read the mockup file:
    - The index.json file can be read as file of text lines. Then
      'file.readlines" can read the file as a list of lines (brute
      force).  This preserves the order of the JSON objects
    - The index.json file can be read as a json file.  Then json.read can
      read the file into a dictionary.  However, this would scramble
      the order of json objects

    Next the in-memory data structure needs to be modified to insert the
    wildcards.

    """
    function_name = 'execute_{0}'.format(base_program_name)
    outfile.write('_TEMPLATE = \\\n')
    global orig_path
    input_file = os.path.join(orig_path, 'index.json')
    exists = os.path.exists (input_file)
    infile = open(input_file, 'r')
    lines =  infile.readlines()
    # TODO: Converter collections resource members into wildcards
    for x in lines:
        outfile.write(x)
    outfile.write('\n\n')
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
