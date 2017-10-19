#  Copyright (c) 2014-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

import os
import sys

from . import pyfdt

MODULES = [
    '/usr/lib/x86_64-linux-gnu/opensc-pkcs11.so',
    '/usr/lib64/opensc-pkcs11.so',
]

ENGINES = [
    '/usr/lib/ssl/engines',
    '/usr/lib64/openssl/engines/',
]

TOKEN_URL = [
    'model=PKCS%2315%20emulated;manufacturer=ZeitControl',
    'serial=%s',
    'token=OpenPGP%20card%20%28User%20PIN%20%28sig%29%29',
    'id=%01',
    'object=Signature%20key',
]


def get_url(serial):
    token = TOKEN_URL
    token[1] = token[1] % (serial)
    return ';'.join(token)


def clear_environment():
    if 'PKCS11_MODULE' in os.environ:
        del os.environ['PKCS11_MODULE']


def enforce_environment():
    if 'PKCS11_MODULE' not in os.environ or not os.environ['PKCS11_MODULE']:
        for mod in MODULES:
            if os.path.exists(mod):
                os.environ['PKCS11_MODULE'] = mod
                break
    if 'PKCS11_MODULE' not in os.environ or not os.environ['PKCS11_MODULE']:
        print('Cannot find OpenSC PKCS#11 module library')
        sys.exit(1)
    if 'OPENSSL_ENGINES' not in os.environ or not os.environ['OPENSSL_ENGINES']:
        for engine in ENGINES:
            if os.path.exists(mod):
                os.environ['PKCS11_MODULE'] = mod
                break
    if 'OPENSSL_ENGINES' not in os.environ or not os.environ['OPENSSL_ENGINES']:
        print('Cannot find OpenSSL engine libraries')
        sys.exit(1)
