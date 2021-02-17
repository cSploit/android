/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Runtime.Serialization.Formatters.Binary;
using BerkeleyDB; 

namespace excs_getting_started {
    public class DatabaseExample {
        /* Set for database load, -l argument. */
        private bool databaseLoad = false;
        /* Set for database read, -r argument. */
        private bool databaseRead = false;
        /* The item to locate if the -i argument is used. */
        private string locateItem = null;

        /* Set for database directory, -b argument. */
        private string dataDir = Environment.CurrentDirectory;
        /* Set for database home, -h argument. */
        private string myDbsPath = Environment.CurrentDirectory;

        public MyDbs myDbs;

        public static void Main(string[] args) {
            try {
                DatabaseExample example = new DatabaseExample();
                example.SetPath();
                example.ParseArgs(args);
                example.SetupDirs();  
                example.myDbs = new MyDbs(example.myDbsPath);

                DatabaseLoader loader = new DatabaseLoader(example.myDbs);
                DatabaseReader reader = new DatabaseReader(example.myDbs);

                if (example.databaseLoad) {
                    /* Load the inventory database. */
                    Console.WriteLine("loading inventory db ...");
                    Console.WriteLine();
                    loader.LoadInventoryDB(example.dataDir);

                    /* Load the vendor database. */
                    Console.WriteLine("loading vendor db ...");
                    Console.WriteLine();
                    loader.LoadVendorDB(example.dataDir);
                }

                if (example.databaseRead) {
                    if (example.locateItem != null) 
                        /* Show an item. */
                        reader.showItem(example.locateItem);
                    else
                        /* Show the full inventory. */
                        reader.showAllInventory();
                }
            }
            catch (Exception e) {
                Console.WriteLine("Test Error.");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        #region Utilities

        private static void Usage() {
            Console.Write("Usage: excs_getting_started ");
            Console.Write("[-l]  [-h <database home>] ");
            Console.WriteLine("[-b <data dir>]");
            Console.WriteLine("or");
            Console.Write("Usage: excs_getting_started ");
            Console.Write("[-r] [-h <database home>] ");
            Console.Write("[-b <data dir>] ");
            Console.WriteLine("[-i <item to locate>]");
            Console.WriteLine("\t -h home (optional; database directory.)");
            Console.WriteLine("\t -b data dir (optional;" + 
                "text input directory.)");
            Console.WriteLine("\t -i item (optional; item to locate.)");
            Environment.Exit(1);
        }

        private void SetPath() {
            String pwd;

            /*
             * This test is meant to be run from:
             *     build_windows\AnyCPU, 
             * in either the Debug or Release directory. The 
             * required core libraries however, are in either:
             *      build_windows\Win32 or build_windows\x64,
             * depending upon the platform.  That location needs
             * to be added to the PATH environment variable for the 
             * P/Invoke calls to work.
             */
            try {
                pwd = Environment.CurrentDirectory;
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                if (IntPtr.Size == 4)
                    pwd = Path.Combine(pwd, "Win32");
                else
                    pwd = Path.Combine(pwd, "x64");
#if DEBUG
                pwd = Path.Combine(pwd, "Debug");
#else
                pwd = Path.Combine(pwd, "Release");
#endif
                pwd += ";" + Environment.GetEnvironmentVariable("PATH");
                Environment.SetEnvironmentVariable("PATH", pwd);
            } catch (Exception e) {
                Console.WriteLine("Unable to set PATH environment variable.");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        /* Extract the command line parameters. */
        private void ParseArgs(string[] args) {
            for (int i = 0; i < args.Length; i++) {
                string s = args[i];
                if (s[0] != '-') {
                    Console.Error.WriteLine("Unrecognized option: " + args[i]);
                    Usage();
                    return;
                }

                switch (s[1]) {
                    case 'l':
                        databaseLoad = true;
                        break;
                    case 'r':
                        databaseRead = true;
                        break;
                    case 'h':
                        if (i == args.Length-1)
                            Usage();
                        myDbsPath = args[++i];
                        break;
                    case 'b':
                        if (i == args.Length-1)
                            Usage();
                        dataDir = args[++i];
                        break;
                    case 'i':
                        if (i == args.Length-1)
                            Usage();
                        locateItem = args[++i];
                        break;
                    default:
                        Console.Error.WriteLine(
                            "Unrecognized option: " + args[i]);
                    Usage();
                    break;
                }
            }
            if ((locateItem != null) && !databaseRead) {
                Usage();
                return;
            }
        }

        private void SetupDirs() {
            if (!Directory.Exists(myDbsPath)) {
                Console.WriteLine("Creating home directory: {0}", myDbsPath);
                try {
                    Directory.CreateDirectory(myDbsPath);
                } catch (Exception e) {
                    Console.WriteLine("Unable to create {0}", myDbsPath);
                    Console.WriteLine(e.Message);
                    throw e;;
                }
            }
            if (!Directory.Exists(dataDir)) {
                Console.WriteLine("Creating data directory: {0}", dataDir);
                try {
                    Directory.CreateDirectory(dataDir);
                } catch (Exception e) {
                    Console.WriteLine("Unable to create {0}", dataDir);
                    Console.WriteLine(e.Message);
                    throw e;
                }
            }
        }

    #endregion Utilities
    }
}
