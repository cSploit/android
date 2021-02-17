/**
 * @copyright
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 * @endcopyright
 *
 * @file SVNBase.h
 * @brief Interface of the class SVNBase
 */

#ifndef SVNBASE_H
#define SVNBASE_H

#include <jni.h>
#include "Pool.h"

class SVNBase
{
 public:
  SVNBase();
  virtual ~SVNBase();

  /**
   * Return @c this as a @c jlong.
   *
   * @since 1.4.0
   */
  jlong getCppAddr() const;

  /**
   * Deletes this C++ peer object, and clears the memory address of
   * the corresponding Java object @a jthis which points to it.
   *
   * @since 1.4.0
   */
  virtual void dispose() = 0;

  /**
   * This method should never be called, as @c dispose() should be
   * called explicitly.  When @c dispose() fails to be called, this
   * method assures that this C++ peer object has been enqueued for
   * deletion.
   *
   * @since 1.4.0
   */
  void finalize();

 protected:
  /**
   * Return the value of the @c cppAddr instance field from the @a
   * jthis Java object, or @c 0 if an error occurs, or the address
   * otherwise can't be determined.  @a fid is expected to point to
   * 0 if not already known, in which case it's looked up using @a
   * className.
   *
   * @since 1.4.0
   */
  static jlong findCppAddrForJObject(jobject jthis, jfieldID *fid,
                                     const char *className);

  /**
   * Deletes @c this, then attempts to null out the @c jthis.cppAddr
   * instance field on the corresponding Java object.
   *
   * @since 1.4.0
   */
  void dispose(jfieldID *fid, const char *className);

  /**
   * A pointer to the parent java object.  This is not valid across JNI
   * method invocations, and so should be set in each one.
   */
  jobject jthis;

 private:
  /**
   * If the value pointed to by @a fid is zero, find the @c jfieldID
   * for the @c cppAddr instance field of @c className.
   *
   * @since 1.4.0
   */
  static void findCppAddrFieldID(jfieldID *fid, const char *className,
                                 JNIEnv *env);

protected:
    SVN::Pool pool;
};

#endif // SVNBASE_H
