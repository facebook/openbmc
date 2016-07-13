/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// This code requires the ajv package from npm and should be run by Node.js.
var Ajv     = require('ajv');
var assert  = require('assert');

var ajv = Ajv({
  allErrors: true,
  // add all the necessary schemas here
  schemas: [
    require('./Attribute.schema.json'),
    require('./SensorAttribute.schema.json'),
    require('./SensorObject.schema.json'),
    require('./Object.schema.json'),
    require('./SensorDevice.schema.json'),
    require('./SensorInfo.schema.json'),
  ]
});

// set default json file to validate the schema
var jsonFileName = './SensorInfo.json';
if (process.argv.length > 2) {
  // get json file name from input
  // should be prefixed with ./ to distinguish it from pkg
  jsonFileName = process.argv[2];
}

var data = require(jsonFileName);
// provide the id of the main json schema for validation
var validate = ajv.getSchema("http://org.openbmc/schemas/SensorInfo.schema.json#");
var valid = validate(data);
if (!valid) {
  console.log(validate.errors);
} else {
  console.log(jsonFileName + ' validated Schema');
}
