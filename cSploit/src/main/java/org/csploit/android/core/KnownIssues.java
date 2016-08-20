package org.csploit.android.core;

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
   * check for known issue using Java
   */
  public void check() {
    //TODO
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

    BufferedReader reader = null;
    try {
      reader = new BufferedReader(new FileReader(file));

      while((line = reader.readLine()) != null) {
        line = line.trim();

        if(line.isEmpty() || line.startsWith("#"))
          continue;

        Integer issue = Integer.parseInt(line);

        if(!foundIssues.contains(issue)) {
          foundIssues.add(issue);
          Logger.info(String.format("issue #%d loaded from file", issue));
        }
      }

    } catch (FileNotFoundException e) {
      // ignored
    } catch (IOException e) {
      Logger.warning(String.format("unable to read from '%s': %s", file, e.getMessage()));
    } catch (NumberFormatException e) {
      Logger.error(String.format("unable to parse '%s' as number.", line));
    } finally {
      if (reader != null) {
        try {
          reader.close();
        } catch (IOException e) {
          // Nothing else matters
        }
      }
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
