/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	public class CSharpTestFixture
	{
		protected internal string testFixtureName;
		protected internal string testFixtureHome;
		protected internal string testName;
		protected internal string testHome;

		public virtual void SetUpTestfixture()
		{
			Console.WriteLine("----  TestClass Started: " + testFixtureName + ".cs ----");
			testFixtureHome = "./TestOut/" + testFixtureName;
			Configuration.ClearDir(testFixtureHome);
		}

		[TestFixtureTearDown]
		public void TearDownTestfixture()
		{
			Console.WriteLine("----  TestClass Finished: " + testFixtureName + ".cs ----");
		}

		public void SetUpTest(bool clean)
		{
			Console.WriteLine("    ====  TestCase Started: " + testName + " ====");
			testHome = testFixtureHome + "/" + testName;
			if (clean == true)
				Configuration.ClearDir(testHome);
		}

		[TearDown]
		public void TearDownTest()
		{
			Console.WriteLine("    ====  TestCase Finished: " + testName + " ====");
		}
	}
}
