package SQLite;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Internal class implementing java.io.InputStream on
 * SQLite 3.4.0 incremental blob I/O interface.
 */

class BlobR extends InputStream {

    /**
     * Blob instance
     */

    private Blob blob;

    /**
     * Read position, file pointer.
     */

    private int pos;

    /**
     * Contruct InputStream from blob instance.
     */

    BlobR(Blob blob) {
	this.blob = blob;
	this.pos = 0;
    }

    /**
     * Return number of available bytes for reading.
     * @return available input bytes
     */

    public int available() throws IOException {
	int ret = blob.size - pos;
	return (ret < 0) ? 0 : ret;
    }

    /**
     * Mark method; dummy to satisfy InputStream class.
     */

    public void mark(int limit) {
    }

    /**
     * Reset method; dummy to satisfy InputStream class.
     */

    public void reset() throws IOException {
    }

    /**
     * Mark support; not for this class.
     * @return always false
     */

    public boolean markSupported() {
	return false;
    }

    /**
     * Close this blob InputStream.
     */

    public void close() throws IOException {
        blob.close();
	blob = null;
	pos = 0;
    }

    /**
     * Skip over blob data.
     */

    public long skip(long n) throws IOException {
	long ret = pos + n;
	if (ret < 0) {
	    ret = 0;
	    pos = 0;
	} else if (ret > blob.size) {
	    ret = blob.size;
	    pos = blob.size;
	} else {
	    pos = (int) ret;
	}
	return ret;
    }

    /**
     * Read single byte from blob.
     * @return byte read
     */

    public int read() throws IOException {
	byte b[] = new byte[1];
	int n = blob.read(b, 0, pos, b.length);
	if (n > 0) {
	    pos += n;
	    return b[0];
	}
	return -1;
    }

    /**
     * Read byte array from blob.
     * @param b byte array to be filled
     * @return number of bytes read
     */

    public int read(byte b[]) throws IOException {
	int n = blob.read(b, 0, pos, b.length);
	if (n > 0) {
	    pos += n;
	    return n;
	}
	return -1;
    }

    /**
     * Read slice of byte array from blob.
     * @param b byte array to be filled
     * @param off offset into byte array
     * @param len length to be read
     * @return number of bytes read
     */

    public int read(byte b[], int off, int len) throws IOException {
	if (off + len > b.length) {
	    len = b.length - off;
	}
	if (len < 0) {
	    return -1;
	}
	if (len == 0) {
	    return 0;
	}
	int n = blob.read(b, off, pos, len);
	if (n > 0) {
	    pos += n;
	    return n;
	}
	return -1;
    }
}

/**
 * Internal class implementing java.io.OutputStream on
 * SQLite 3.4.0 incremental blob I/O interface.
 */

class BlobW extends OutputStream {

    /**
     * Blob instance
     */

    private Blob blob;

    /**
     * Read position, file pointer.
     */

    private int pos;

    /**
     * Contruct OutputStream from blob instance.
     */

    BlobW(Blob blob) {
	this.blob = blob;
	this.pos = 0;
    }

    /**
     * Flush blob; dummy to satisfy OutputStream class.
     */

    public void flush() throws IOException {
    }

    /**
     * Close this blob OutputStream.
     */

    public void close() throws IOException {
        blob.close();
	blob = null;
	pos = 0;
    }

    /**
     * Write blob data.
     * @param v byte to be written at current position.
     */

    public void write(int v) throws IOException {
	byte b[] = new byte[1];
	b[0] = (byte) v;
	pos += blob.write(b, 0, pos, 1);
    }

    /**
     * Write blob data.
     * @param b byte array to be written at current position.
     */

    public void write(byte[] b) throws IOException {
	if (b != null && b.length > 0) {
	    pos += blob.write(b, 0, pos, b.length);
	}
    }

    /**
     * Write blob data.
     * @param b byte array to be written.
     * @param off offset within byte array
     * @param len length of data to be written
     */

    public void write(byte[] b, int off, int len) throws IOException {
	if (b != null) {
	    if (off + len > b.length) {
		len = b.length - off;
	    }
	    if (len <= 0) {
		return;
	    }
	    pos += blob.write(b, off, pos, len);
	}
    }
}

/**
 * Class to represent SQLite3 3.4.0 incremental blob I/O interface.
 *
 * Note, that all native methods of this class are
 * not synchronized, i.e. it is up to the caller
 * to ensure that only one thread is in these
 * methods at any one time.
 */

public class Blob {

    /**
     * Internal handle for the SQLite3 blob.
     */

    private long handle = 0;

    /**
     * Cached size of blob, setup right after blob
     * has been opened.
     */

    protected int size = 0;

    /**
     * Return InputStream for this blob
     * @return InputStream
     */

    public InputStream getInputStream() {
	return new BlobR(this);
    }

    /**
     * Return OutputStream for this blob
     * @return OutputStream
     */

    public OutputStream getOutputStream() {
	return new BlobW(this);
    }

    /**
     * Close blob.
     */

    public native void close();

    /**
     * Internal blob write method.
     * @param b byte array to be written
     * @param off offset into byte array
     * @param pos offset into blob
     * @param len length to be written
     * @return number of bytes written to blob
     */

    native int write(byte[] b, int off, int pos, int len) throws IOException;

    /**
     * Internal blob read method.
     * @param b byte array to be written
     * @param off offset into byte array
     * @param pos offset into blob
     * @param len length to be written
     * @return number of bytes written to blob
     */

    native int read(byte[] b, int off, int pos, int len) throws IOException;

    /**
     * Destructor for object.
     */

    protected native void finalize();

    /**
     * Internal native initializer.
     */

    private static native void internal_init();

    static {
	internal_init();
    }
}
