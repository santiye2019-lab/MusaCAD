package com.musa.cad;

/**
 * Hard and soft deadlines for a three-batch visual CAD sweep.
 *
 * The model HTTPS read limit is 90 seconds, so an idle *notice* must not
 * silently discard a still-running request after only 75 seconds.
 * The main sweep must stop starting new batches well before the UI hard cap.
 */
public final class MusaAiVisionTimeBudget {
    public static final long MODEL_READ_LIMIT_MS=90_000L;
    public static final long RENDER_BATCH_LIMIT_MS=25_000L;
    public static final long IDLE_NOTICE_MS=120_000L;
    public static final long SWEEP_START_DEADLINE_MS=480_000L;
    public static final long PANEL_HARD_DEADLINE_MS=720_000L;

    public static boolean mayStartNextBatch(long elapsedMs){
        return elapsedMs>=0L&&elapsedMs<SWEEP_START_DEADLINE_MS;
    }

    public static boolean mayRetryBatch(long elapsedMs){
        return mayStartNextBatch(elapsedMs);
    }

    private MusaAiVisionTimeBudget(){}
}
