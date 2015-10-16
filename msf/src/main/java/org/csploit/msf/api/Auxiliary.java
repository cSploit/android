package org.csploit.msf.api;

/**
 * The auxiliary class acts as a base class for all modules that perform
 * reconnaissance, retrieve data, brute force logins, or any other action
 * that doesn't fit our concept of an 'exploit' (involving payloads and
 * targets and whatnot).
 */
public interface Auxiliary extends Module {
}
