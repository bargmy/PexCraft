package net.minecraft.src;

class ThreadRunIsoClient extends Thread {
	final CanvasIsomPreview field_1197_a;

	ThreadRunIsoClient(CanvasIsomPreview var1) {
		this.field_1197_a = var1;
	}

	public void run() {
		while(CanvasIsomPreview.isRunning(this.field_1197_a)) {
			this.field_1197_a.render();

			try {
				Thread.sleep(1L);
			} catch (Exception var2) {
			}
		}

	}
}
