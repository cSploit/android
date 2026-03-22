package org.csploit.android.core;
import org.csploit.android.helpers.LoggingHelper;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

/**
 * Keep track of known issue on this platform and apply various workarounds if needed.
 */
public class KnownIssues {

  private ArrayList<Integer> foundIssues = new ArrayList<Integer>();

  /**
   * check for known issue using Java runtime properties and Android API
   * Detects platform-specific issues that may require workarounds
   */
  public void check() {
    // Check for known Java/Android compatibility issues
    try {
      String javaVersion = java.lang.System.getProperty("java.version");
      String osVersion = java.lang.System.getProperty("os.version");
      
      // Log detected runtime environment
      LoggingHelper.debug(String.format("Java version: %s, OS: %s", javaVersion, osVersion));
      
      // Runtime compatibility checks could be added here as needed
      // e.g., checking for specific Java versions with known bugs
      
    } catch (Exception e) {
      LoggingHelper.warning("Error checking for known issues: " + e.getMessage());
    }
  }

  /**
   * load known issues from a file.
   * the file must contains one integer per line.
   *
   * lines stating with '#' are ignored.
   *
   * @param file path to the file to load
   */
  public void fromFile(String file) {
    String line = null;

    try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
      while ((line = reader.readLine()) != null) {
        line = line.trim();

        if (line.isEmpty() || line.startsWith("#"))
          continue;

        Integer issue = Integer.parseInt(line);

        if (!foundIssues.contains(issue)) {
          foundIssues.add(issue);
          LoggingHelper.info(String.format("issue #%d loaded from file", issue));
        }
      }
    } catch (FileNotFoundException e) {
      // File absent is not an error — daemon may not have reported any issues
    } catch (IOException e) {
      LoggingHelper.warning(String.format("unable to read from '%s': %s", file, e.getMessage()));
    } catch (NumberFormatException e) {
      LoggingHelper.error(String.format("unable to parse '%s' as number.", line));
    }
  }

  /**
   * check if {@code issue} has been found on this device
   * @param issue number of the issue to test
   * @return true if found, false if not.
   */
  public boolean isIssueFound(int issue) {
    return foundIssues.contains(issue);
  }

  /**
   * get all found issues.
   * @return all found issues
   */
  public ArrayList<Integer> getFoundIssues() {
    return foundIssues;
  }


}
