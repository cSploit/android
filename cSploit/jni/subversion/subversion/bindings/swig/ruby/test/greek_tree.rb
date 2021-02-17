# ====================================================================
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
# ====================================================================

module SvnTestUtil
  class Greek
    TREE = [
            #  relative path , contents(nil means directory)
            ["iota"        , "This is the file 'iota'.\n"   ],
            ["A"           , nil                            ],
            ["A/mu"        , "This is the file 'mu'.\n"     ],
            ["A/B"         , nil                            ],
            ["A/B/lambda"  , "This is the file 'lambda'.\n" ],
            ["A/B/E"       , nil                            ],
            ["A/B/E/alpha" , "This is the file 'alpha'.\n"  ],
            ["A/B/E/beta"  , "This is the file 'beta'.\n"   ],
            ["A/B/F"       , nil                            ],
            ["A/C"         , nil                            ],
            ["A/D"         , nil                            ],
            ["A/D/gamma"   , "This is the file 'gamma'.\n"  ],
            ["A/D/G"       , nil                            ],
            ["A/D/G/pi"    , "This is the file 'pi'.\n"     ],
            ["A/D/G/rho"   , "This is the file 'rho'.\n"    ],
            ["A/D/G/tau"   , "This is the file 'tau'.\n"    ],
            ["A/D/H"       , nil                            ],
            ["A/D/H/chi"   , "This is the file 'chi'.\n"    ],
            ["A/D/H/psi"   , "This is the file 'psi'.\n"    ],
            ["A/D/H/omega" , "This is the file 'omega'.\n"  ]
           ]

    TREE.each do |path, contents|
      const_set(path.split("/").last.upcase, path)
    end

    def initialize(tmp_path, wc_path, repos_uri)
      @tmp_path = tmp_path
      @wc_path = wc_path
      @repos_uri = repos_uri
    end

    def setup(context)
      TREE.each do |path, contents|
        entry = File.expand_path(File.join(@tmp_path, path))
        if contents
          File.open(entry, 'w') {|f| f.print(contents)}
        else
          FileUtils.mkdir(entry)
        end
      end

      context.import(@tmp_path, @repos_uri)
      context.update(@wc_path)
    end

    def path(greek)
      File.join(@wc_path, resolve(greek))
    end

    def uri(greek)
      "#{@repos_uri}/#{resolve(greek)}"
    end

    def resolve(greek)
      self.class.const_get(greek.to_s.upcase)
    end
  end
end
