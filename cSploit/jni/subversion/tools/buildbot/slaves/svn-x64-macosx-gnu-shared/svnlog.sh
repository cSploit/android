#!/bin/bash
#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#

# upload file to server
FILENAME=tests-`date +%Y%m%d%H%M`.log.tgz
tar -czf $FILENAME tests.log
ftp -n www.mobsol.be < ../ftpscript 
rm $FILENAME

echo "Logs of the testrun can be found here: http://www.mobsol.be/logs/osx10.4-gcc4.0.1-ia32/$FILENAME"

exit 0
