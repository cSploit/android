#!/bin/sh
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

if test $# != 3; then
  echo "USAGE: $0 DATABASE_USER DATABASE_PASSWORD MYSQL_DATABASE"
  exit 1
fi

dbuser="$1"
dbpass="$2"
dbdb="$3"

q1='select issues.issue_id, issue_type, user1.LOGIN_NAME "reporter",
       user2.LOGIN_NAME "assigned_to", target_milestone, creation_ts,
       max(issue_when) "resolved_ts", resolution, short_desc
  from issues left join issues_activity
           on issues.issue_id=issues_activity.issue_id and newvalue="RESOLVED",
       profiles prof1,
       profiles prof2 left join tigris.HELM_USER user1
           on user1.USER_ID=prof1.helm_user_id
         left join tigris.HELM_USER user2
           on user2.USER_ID=prof2.helm_user_id
  where prof1.userid=reporter and prof2.userid=assigned_to
  group by issues.issue_id
  order by issues.issue_id'

q2='select issues.issue_id, issue_type, user1.LOGIN_NAME "reporter",
       user2.LOGIN_NAME "assigned_to", target_milestone, creation_ts,
       max(issue_when) "resolved_ts", resolution, short_desc,
       priority
  from issues left join issues_activity
           on issues.issue_id=issues_activity.issue_id and newvalue="RESOLVED",
       profiles prof1,
       profiles prof2 left join tigris.HELM_USER user1
           on user1.USER_ID=prof1.helm_user_id
         left join tigris.HELM_USER user2
           on user2.USER_ID=prof2.helm_user_id
  where prof1.userid=reporter and prof2.userid=assigned_to
  group by issues.issue_id
  order by issues.issue_id'

mysql --batch -e "use $dbdb; $q1" --user=$dbuser --password=$dbpass --silent > iz-data/query-set-1.tsv
mysql --batch -e "use $dbdb; $q2" --user=$dbuser --password=$dbpass --silent > iz-data/query-set-2.tsv
