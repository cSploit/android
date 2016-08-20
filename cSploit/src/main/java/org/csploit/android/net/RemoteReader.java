package org.csploit.android.net;

import org.apache.commons.compress.utils.IOUtils;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * take advantage of persistent HTTP connections
 * by use 1 thread for each host.
 */
public class RemoteReader implements Runnable {

  /**
   * receive fetched content
   */
  public interface Receiver {
    void onContentFetched(byte[] content);
    void onError(byte[] description);
  }

  /**
   * receive the job end notification
   */
  public interface EndReceiver {
    void onEnd();
  }

  /**
   * Asynchronous fetch job.
   *
   * waits that all pending urls get fetched and received.
   * is possible to add more urls while the job is running using {@link org.csploit.android.net.RemoteReader.Job#add(String, org.csploit.android.net.RemoteReader.Receiver) add}
   */
  public static class Job implements Future {
    private final Queue<Task> taskQueue = new ArrayDeque<Task>(5);
    private final EndReceiver endReceiver;
    private JobStatus status = JobStatus.RUNNING;

    @Override
    public boolean cancel(boolean mayInterruptIfRunning) {
      if(status == JobStatus.FINISHED || status == JobStatus.CANCELLED)
        return false;

      synchronized (taskQueue) {
        for (Task t : taskQueue) {
          try {
            RemoteReader.fromUrl(t.getUrl()).remove(t);
          } catch (MalformedURLException e) {
            System.errorLogging(e);
          }
        }
        status = JobStatus.CANCELLED;
        taskQueue.clear();
        taskQueue.notify();
      }
      endReceiver.onEnd();
      return true;
    }

    @Override
    public boolean isCancelled() {
      return status == JobStatus.CANCELLED;
    }

    @Override
    public boolean isDone() {
      return status != JobStatus.RUNNING;
    }

    @Override
    public Object get() throws InterruptedException, ExecutionException {
      synchronized (taskQueue) {
        while(taskQueue.size() > 0)
          taskQueue.wait();
      }

      return null;
    }

    @Override
    public Object get(long l, TimeUnit timeUnit) throws InterruptedException, ExecutionException, TimeoutException {

      if(isDone())
        return null;

      synchronized (taskQueue) {
        taskQueue.wait(timeUnit.toMillis(l));

        if(!isDone())
          throw new TimeoutException();
      }

      return null;
    }

    private enum JobStatus {
      RUNNING,
      CANCELLED,
      FINISHED
    }

    public Job(EndReceiver endReceiver) {
      this.endReceiver = endReceiver;
    }

    public void add(String url, Receiver receiver) throws MalformedURLException, IllegalStateException {
      RemoteReader reader = RemoteReader.fromUrl(url);
      Task task = new Task(url, this, receiver);

      synchronized (taskQueue) {
        if(status != JobStatus.RUNNING) {
          throw new IllegalStateException("RemoteReader Job " + (status == JobStatus.CANCELLED ? "cancelled" : "finished"));
        }
        taskQueue.add(task);
      }

      reader.add(task);
    }

    public void onTaskDone(Task t) {
      boolean notifyJobEnd = false;

      synchronized (taskQueue) {
        if(taskQueue.remove(t) && taskQueue.isEmpty()) {
          taskQueue.notify();
          if(status == JobStatus.RUNNING) {
            status = JobStatus.FINISHED;
            notifyJobEnd = true;
          }
        }
      }

      if(notifyJobEnd)
        endReceiver.onEnd();
    }
  }

  /**
   * send the fetched content to the receiver.
   */
  private static class Notifier implements Runnable {
    private final Task task;
    private final byte[] content;
    private final boolean isError;

    /**
     * Send fetched content to the {@link RemoteReader.Receiver receiver}
     *
     * @param task the finished task to notify
     * @param content fetched content
     * @param isError is this content about an error ?
     */
    public Notifier(Task task, byte[] content, boolean isError) {
      this.task = task;
      this.content = content;
      this.isError = isError;
    }

    @Override
    public void run() {
      Thread.currentThread().setName("Notifier[" + task.getUrl() + "]");
      if (isError) {
        task.getReceiver().onError(content);
      } else {
        task.getReceiver().onContentFetched(content);
      }
      task.onDone();
    }
  }

  /**
   * a task is a single fetch operation.
   *
   * it's made by 2 steps:
   * <ul>
   *   <li>download the content</li>
   *   <li>notify about the download content</li>
   * </ul>
   */
  private static class Task {
    private final String url;
    private final Job owner;
    private final Receiver receiver;

    public Task(String url, Job owner, Receiver receiver) {
      this.url = url;
      this.owner = owner;
      this.receiver = receiver;
    }

    public Task(String url, Receiver receiver) {
      this.url = url;
      this.receiver = receiver;
      this.owner = null;
    }

    public String getUrl() {
      return url;
    }

    public Receiver getReceiver() {
      return receiver;
    }

    public void onDone() {
      if(owner != null)
        owner.onTaskDone(this);
    }
  }

  private boolean running;
  private final String host;
  private static final LinkedList<RemoteReader> readers = new LinkedList<RemoteReader>();
  private static final ExecutorService _readers = Executors.newCachedThreadPool();
  private static final ExecutorService notifiers = Executors.newCachedThreadPool();
  private final Queue<Task> tasks = new LinkedList<Task>();

  static {
    HttpURLConnection.setFollowRedirects(true);
  }

  private RemoteReader(String host) {
    this.host = host;
  }

  private String getHost() {
    return host;
  }

  public static RemoteReader fromHost(String host) {
    synchronized (readers) {
      for (RemoteReader reader : readers) {
        if (reader.getHost().equals(host)) {
          return reader;
        }
      }

      RemoteReader r = new RemoteReader(host);
      _readers.submit(r);
      readers.add(r);
      return r;
    }
  }

  public static RemoteReader fromUrl(String url) throws MalformedURLException {
    return fromHost(new URL(url).getHost());
  }

  public static byte[] fetch(final String url) throws IOException {
    class BlockingReceiver implements Receiver {

      private boolean done = false;
      private byte[] content = null;
      private String errorMessage = null;

      @Override
      public void onContentFetched(byte[] content) {
        this.content = content;
        this.done = true;
        synchronized (this) {
          this.notify();
        }
      }

      @Override
      public void onError(byte[] description) {
        this.errorMessage = new String(description);
        this.done = true;
        synchronized (this) {
          this.notify();
        }
      }

      public byte[] getContent() {
        return content;
      }

      public boolean isDone() {
        return done;
      }

      public String getErrorMessage() {
        return errorMessage;
      }
    }

    RemoteReader reader = fromUrl(url);
    BlockingReceiver r = new BlockingReceiver();
    Task t = new Task(url, r);
    reader.addFirst(t);

    try {
      synchronized (r) {
        while(!r.isDone())
          r.wait();
      }
    } catch (InterruptedException e) {
      throw new IOException("Interrupted while fetching content");
    }

    byte[] res = r.getContent();

    if(res == null)
      throw new IOException(r.getErrorMessage());

    return res;
  }

  public static void terminateAll() {
    synchronized (readers) {
      for(RemoteReader r : readers) {
        r.terminate();
      }
    }
  }

  private void addFirst(Task task) {
    synchronized (tasks) {
      ((Deque<Task>) tasks).addFirst(task);
      tasks.notify();
    }
  }

  public void add(Task task) {
    synchronized (tasks) {
      tasks.add(task);
      tasks.notify();
    }
  }

  public void remove(Task task) {
    synchronized (tasks) {
      tasks.remove(task);
    }
  }

  public void run() {
    URLConnection connection = null;
    Task task = null;
    boolean isError;
    InputStream stream;
    Notifier notifier;
    URL url;

    running = true;

    Thread.currentThread().setName("RemoteReader[" + host + "]");

    Logger.debug("RemoteReader[" + host +"] started");

    while(running) {

      try {
        synchronized (tasks) {
          while(running && (task = tasks.poll()) == null) {
            tasks.wait();
          }
        }
      } catch (InterruptedException e) {
        Logger.warning("RemoteReader[" + host +"] interrupted");
        break;
      }

      if(task == null) break;

      isError = false;
      notifier = null;
      stream = null;

      try {
        url = new URL(task.getUrl());
      } catch (MalformedURLException e) {
        notifiers.execute(new Notifier(task, ("Bad URL: " + task.getUrl()).getBytes(), true));
        continue;
      }

      if(!url.getHost().equals(host)) {
        Logger.error(String.format("RemoteReader[%s]: URL '%s' does not belong to me", host, task.getUrl()));
        notifiers.execute(new Notifier(task, "Host mismatch".getBytes(), true));
        continue;
      }

      Logger.info("fetching '" + url.toString() + "'");

      try {
        connection = url.openConnection();
        stream = connection.getInputStream();
      } catch (IOException e) {
        if (connection != null && connection instanceof HttpURLConnection) {
          stream = ((HttpURLConnection) connection).getErrorStream();
          isError = true;
        } else {
          notifier = new Notifier(task, e.getMessage().getBytes(), true);
        }
      }

      if (notifier == null) {
        try {
          notifier = new Notifier(task, IOUtils.toByteArray(stream), isError);
          stream.close();
        } catch (IOException e) {
          notifier = new Notifier(task, e.getMessage().getBytes(), true);
        } catch (IllegalStateException e) {
          notifier = new Notifier(task, e.getMessage().getBytes(), true);
        }
      }

      notifiers.execute(notifier);
    }

    Logger.debug("RemoteReader[" + host +"] quitting");

    running = false;

    synchronized (readers) {
      readers.remove(this);
    }
  }

  public void terminate() {
    running = false;
    synchronized (tasks) {
      tasks.notify();
    }
  }
}
