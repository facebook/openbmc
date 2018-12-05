# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.


from distutils.core import setup

setup(
    name='pypartition',
    version='1.0',
    url='https://github.com/facebook/openbmc',
    description='Tool for checking and updating OpenBMC images.',
    license='GPLv2',
    scripts=['check_image.py', 'improve_system.py'],
    py_modules=['partition', 'system', 'virtualcat']
)
