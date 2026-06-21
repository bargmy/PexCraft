package ir.hienob.pexcraft.tv;

import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class PexCraftActivity extends SDLActivity {
    private static final String TAG = "PexCraftTV";

    public native boolean nativeOnTvKey(int keyCode, boolean down);

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

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
            boolean down = action == KeyEvent.ACTION_DOWN;
            if (down && event.getRepeatCount() == 0) {
                Log.d(TAG, KeyEvent.keyCodeToString(keyCode) + " = " + keyCode);
            }
            if (isTvRemoteKey(keyCode)) {
                try {
                    if (nativeOnTvKey(keyCode, down)) return true;
                } catch (UnsatisfiedLinkError ignored) {
                    // SDL has not loaded libmain yet; fall through to default handling.
                }
            }
        }
        return super.dispatchKeyEvent(event);
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
