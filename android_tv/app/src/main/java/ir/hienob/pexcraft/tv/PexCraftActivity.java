package ir.hienob.pexcraft.tv;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.net.HttpURLConnection;

public class PexCraftActivity extends SDLActivity {
    private static final String TAG = "PexCraftTV";

    private static final int CLASSIC_IDLE = 0;
    private static final int CLASSIC_DOWNLOADING = 1;
    private static final int CLASSIC_EXTRACTING = 2;
    private static final int CLASSIC_DONE = 3;
    private static final int CLASSIC_ERROR = 4;

    private static final int CLASSIC_SIZE_UNKNOWN = 0;
    private static final int CLASSIC_SIZE_FETCHING = 1;
    private static final int CLASSIC_SIZE_READY = 2;
    private static final int CLASSIC_SIZE_ERROR = 3;

    private static final Object classicLock = new Object();
    private static volatile int classicDownloadState = CLASSIC_IDLE;
    private static volatile int classicDownloadProgress = 0;
    private static volatile String classicDownloadStatus = "";
    private static volatile String classicDownloadError = "";
    private static volatile int classicSizeState = CLASSIC_SIZE_UNKNOWN;
    private static volatile int classicSizeBytes = 0;
    private static volatile boolean classicCancelRequested = false;


    public native boolean nativeOnTvKey(int keyCode, boolean down, int source);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        copyBundledGameAssets();
        super.onCreate(savedInstanceState);
        TvChannelPublisher.publishDefaultChannel(this);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    public boolean startClassicPackDownload(final String url, final String outPath) {
        synchronized (classicLock) {
            if (classicDownloadState == CLASSIC_DOWNLOADING || classicDownloadState == CLASSIC_EXTRACTING) return true;
            classicDownloadState = CLASSIC_DOWNLOADING;
            classicDownloadProgress = 0;
            classicDownloadStatus = "Downloading client.jar...";
            classicDownloadError = "";
            classicCancelRequested = false;
        }

        Thread th = new Thread(new Runnable() {
            @Override public void run() {
                downloadClassicPack(url, outPath);
            }
        }, "PexCraftClassicDownload");
        th.setDaemon(true);
        th.start();
        return true;
    }

    public void cancelClassicPackDownload() {
        classicCancelRequested = true;
        classicDownloadStatus = "Canceling...";
    }


    public boolean downloadFileBlocking(final String url, final String outPath) {
        HttpURLConnection conn = null;
        File outFile = new File(outPath);
        File parent = outFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            Log.w(TAG, "Could not create download directory: " + parent);
            return false;
        }
        try {
            if (outFile.exists() && !outFile.delete()) {
                Log.w(TAG, "Could not delete old download: " + outFile);
            }
            conn = openClassicConnection(url);
            conn.setRequestMethod("GET");
            conn.connect();
            int response = conn.getResponseCode();
            if (response < 200 || response >= 300) {
                Log.w(TAG, "HTTP " + response + " while downloading " + url);
                return false;
            }
            byte[] buf = new byte[32 * 1024];
            try (InputStream in = conn.getInputStream(); OutputStream out = new FileOutputStream(outFile)) {
                int n;
                while ((n = in.read(buf)) >= 0) {
                    if (n > 0) out.write(buf, 0, n);
                }
            }
            return outFile.length() > 0;
        } catch (Throwable t) {
            Log.e(TAG, "File download failed: " + url, t);
            if (outFile.exists() && !outFile.delete()) Log.w(TAG, "Could not delete failed download: " + outFile);
            return false;
        } finally {
            if (conn != null) conn.disconnect();
        }
    }

    public int getClassicPackDownloadState() { return classicDownloadState; }
    public int getClassicPackDownloadProgress() { return classicDownloadProgress; }
    public String getClassicPackDownloadStatus() { return classicDownloadStatus == null ? "" : classicDownloadStatus; }
    public String getClassicPackDownloadError() { return classicDownloadError == null ? "" : classicDownloadError; }

    public void startClassicPackSizeFetch(final String url) {
        synchronized (classicLock) {
            if (classicSizeState == CLASSIC_SIZE_FETCHING || classicSizeState == CLASSIC_SIZE_READY) return;
            classicSizeState = CLASSIC_SIZE_FETCHING;
            classicSizeBytes = 0;
        }
        Thread th = new Thread(new Runnable() {
            @Override public void run() {
                HttpURLConnection conn = null;
                try {
                    conn = openClassicConnection(url);
                    conn.setRequestMethod("HEAD");
                    conn.connect();
                    int len = conn.getContentLength();
                    if (len > 0) {
                        classicSizeBytes = len;
                        classicSizeState = CLASSIC_SIZE_READY;
                    } else {
                        classicSizeState = CLASSIC_SIZE_ERROR;
                    }
                } catch (Throwable t) {
                    Log.w(TAG, "Classic pack size fetch failed", t);
                    classicSizeState = CLASSIC_SIZE_ERROR;
                } finally {
                    if (conn != null) conn.disconnect();
                }
            }
        }, "PexCraftClassicSize");
        th.setDaemon(true);
        th.start();
    }

    public int getClassicPackSizeState() { return classicSizeState; }
    public int getClassicPackSizeBytes() { return classicSizeBytes; }

    /** Decode external texture-pack PNGs for native code.
     *  Output layout: little-endian width, little-endian height, then RGBA8888 pixels. */
    public byte[] decodePngToRgba(String path) {
        Bitmap bmp = null;
        try {
            bmp = BitmapFactory.decodeFile(path);
            if (bmp == null) return null;
            int w = bmp.getWidth();
            int h = bmp.getHeight();
            if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return null;
            int[] pixels = new int[w * h];
            bmp.getPixels(pixels, 0, w, 0, 0, w, h);
            byte[] out = new byte[8 + w * h * 4];
            out[0] = (byte)(w & 255);
            out[1] = (byte)((w >> 8) & 255);
            out[2] = (byte)((w >> 16) & 255);
            out[3] = (byte)((w >> 24) & 255);
            out[4] = (byte)(h & 255);
            out[5] = (byte)((h >> 8) & 255);
            out[6] = (byte)((h >> 16) & 255);
            out[7] = (byte)((h >> 24) & 255);
            int o = 8;
            for (int p : pixels) {
                out[o++] = (byte)((p >> 16) & 255); // R
                out[o++] = (byte)((p >> 8) & 255);  // G
                out[o++] = (byte)(p & 255);         // B
                out[o++] = (byte)((p >> 24) & 255); // A
            }
            return out;
        } catch (Throwable t) {
            Log.e(TAG, "PNG decode failed: " + path, t);
            return null;
        } finally {
            if (bmp != null) bmp.recycle();
        }
    }

    private static HttpURLConnection openClassicConnection(String urlString) throws IOException {
        HttpURLConnection conn = (HttpURLConnection)new URL(urlString).openConnection();
        conn.setInstanceFollowRedirects(true);
        conn.setConnectTimeout(20000);
        conn.setReadTimeout(60000);
        conn.setRequestProperty("User-Agent", "PEXCRAFT/1.0 AndroidTV");
        return conn;
    }

    private void downloadClassicPack(String urlString, String outPath) {
        HttpURLConnection conn = null;
        File outFile = new File(outPath);
        File parent = outFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            failClassicDownload("Could not create download directory");
            return;
        }

        try {
            if (outFile.exists() && !outFile.delete()) {
                Log.w(TAG, "Could not delete old classic jar: " + outFile);
            }

            classicDownloadStatus = "Connecting to Mojang...";
            classicDownloadProgress = 1;
            conn = openClassicConnection(urlString);
            conn.setRequestMethod("GET");
            conn.connect();
            int response = conn.getResponseCode();
            if (response < 200 || response >= 300) {
                failClassicDownload("HTTP " + response + " while downloading client.jar");
                return;
            }
            int total = conn.getContentLength();
            byte[] buf = new byte[32 * 1024];
            long downloaded = 0;

            try (InputStream in = conn.getInputStream(); OutputStream out = new FileOutputStream(outFile)) {
                int n;
                while ((n = in.read(buf)) >= 0) {
                    if (classicCancelRequested) {
                        if (!outFile.delete()) Log.w(TAG, "Could not delete canceled classic jar: " + outFile);
                        failClassicDownload("Canceled");
                        return;
                    }
                    if (n == 0) continue;
                    out.write(buf, 0, n);
                    downloaded += n;
                    if (total > 0) {
                        int pct = 1 + (int)((downloaded * 84L) / total);
                        if (pct > 85) pct = 85;
                        classicDownloadProgress = pct;
                        classicDownloadStatus = "Downloading client.jar (" + pct + "%)";
                    } else {
                        classicDownloadProgress = 40;
                        classicDownloadStatus = "Downloading client.jar (" + downloaded + " bytes)";
                    }
                }
            }

            if (outFile.length() < 1024) {
                if (!outFile.delete()) Log.w(TAG, "Could not delete empty classic jar: " + outFile);
                failClassicDownload("Downloaded client.jar was empty");
                return;
            }

            classicDownloadProgress = 85;
            classicDownloadStatus = "Downloaded client.jar";
            classicDownloadState = CLASSIC_DONE;
        } catch (Throwable t) {
            Log.e(TAG, "Classic pack download failed", t);
            if (outFile.exists() && !outFile.delete()) Log.w(TAG, "Could not delete failed classic jar: " + outFile);
            failClassicDownload(t.getMessage() != null ? t.getMessage() : t.toString());
        } finally {
            if (conn != null) conn.disconnect();
        }
    }

    private static void failClassicDownload(String message) {
        classicDownloadError = message == null || message.length() == 0 ? "Download failed" : message;
        classicDownloadStatus = "Failed";
        classicDownloadProgress = 0;
        classicDownloadState = CLASSIC_ERROR;
    }


    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
            boolean down = action == KeyEvent.ACTION_DOWN;
            if (down && event.getRepeatCount() == 0) {
                Log.d(TAG, KeyEvent.keyCodeToString(keyCode) + " = " + keyCode);
            }
            if (isTvRemoteKey(keyCode) && isActualTvRemoteEvent(event)) {
                try {
                    if (nativeOnTvKey(keyCode, down, event.getSource())) return true;
                } catch (UnsatisfiedLinkError ignored) {
                    // SDL has not loaded libmain yet; fall through to default handling.
                }
            }
        }
        return super.dispatchKeyEvent(event);
    }


    private static boolean isActualTvRemoteEvent(KeyEvent event) {
        int source = event.getSource();
        // Android mice can synthesize DPAD_CENTER/ENTER for the primary button.
        // Let SDL receive those as mouse events; otherwise a click also jumps.
        if ((source & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE
                || (source & InputDevice.SOURCE_STYLUS) == InputDevice.SOURCE_STYLUS
                || (source & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN
                || (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
            return false;
        }
        if ((source & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) return true;
        InputDevice device = event.getDevice();
        if (device == null) return false;
        int deviceSources = device.getSources();
        if ((deviceSources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE
                || (deviceSources & InputDevice.SOURCE_STYLUS) == InputDevice.SOURCE_STYLUS
                || (deviceSources & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN
                || (deviceSources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (deviceSources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
            return false;
        }
        return (deviceSources & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
    }

    private static boolean isTvRemoteKey(int keyCode) {
        return (keyCode >= KeyEvent.KEYCODE_0 && keyCode <= KeyEvent.KEYCODE_9)
                || keyCode == KeyEvent.KEYCODE_BACK
                || keyCode == KeyEvent.KEYCODE_DPAD_UP
                || keyCode == KeyEvent.KEYCODE_DPAD_DOWN
                || keyCode == KeyEvent.KEYCODE_DPAD_LEFT
                || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT
                || keyCode == KeyEvent.KEYCODE_DPAD_CENTER
                || keyCode == KeyEvent.KEYCODE_ENTER
                || keyCode == KeyEvent.KEYCODE_NUMPAD_ENTER
                || keyCode == KeyEvent.KEYCODE_PROG_RED
                || keyCode == KeyEvent.KEYCODE_PROG_GREEN
                || keyCode == KeyEvent.KEYCODE_PROG_YELLOW
                || keyCode == KeyEvent.KEYCODE_PROG_BLUE;
    }

    private void copyBundledGameAssets() {
        File outRoot = new File(getFilesDir(), "assets");
        File stamp = new File(outRoot, ".pexcraft-assets-v1");
        if (stamp.exists()) return;
        if (!outRoot.exists() && !outRoot.mkdirs()) {
            Log.w(TAG, "Could not create asset output dir: " + outRoot);
            return;
        }
        try {
            copyAssetDir("assets", outRoot);
            if (!stamp.exists()) stamp.createNewFile();
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy bundled assets", e);
        }
    }

    private void copyAssetDir(String assetPath, File outDir) throws IOException {
        String[] children = getAssets().list(assetPath);
        if (children == null || children.length == 0) {
            copyAssetFile(assetPath, outDir);
            return;
        }
        if (!outDir.exists() && !outDir.mkdirs()) {
            throw new IOException("Could not create " + outDir);
        }
        for (String child : children) {
            String childAssetPath = assetPath + "/" + child;
            File childOut = new File(outDir, child);
            String[] grandChildren = getAssets().list(childAssetPath);
            if (grandChildren != null && grandChildren.length > 0) copyAssetDir(childAssetPath, childOut);
            else copyAssetFile(childAssetPath, childOut);
        }
    }

    private void copyAssetFile(String assetPath, File outFile) throws IOException {
        File parent = outFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Could not create " + parent);
        }
        try (InputStream in = getAssets().open(assetPath); OutputStream out = new FileOutputStream(outFile)) {
            byte[] buf = new byte[16 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
    }
}
