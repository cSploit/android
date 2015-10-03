package org.csploit.msf;

import java.util.HashMap;

/**
 * This class contains zero or more abstract jobs that can be enumerated and
 * stopped in a generic fashion.  This is used to provide a mechanism for
 * keeping track of arbitrary contexts that may or may not require a dedicated
 * thread.
 */
public class JobContainer extends HashMap<Integer, Job> {
}
