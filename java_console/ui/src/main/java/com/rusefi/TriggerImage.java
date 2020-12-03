package com.rusefi;

import com.rusefi.config.generated.Fields;
import com.rusefi.trigger.WaveState;
import com.rusefi.ui.engine.UpDownImage;
import com.rusefi.ui.util.FrameHelper;
import com.rusefi.ui.util.UiUtils;
import com.rusefi.waves.EngineReport;
import org.jetbrains.annotations.NotNull;

import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.stream.Collectors;

/**
 * This utility produces images of trigger signals supported by rusEFI
 * <p>
 * 06/23/15
 * Andrey Belomutskiy, (c) 2013-2020
 */
public class TriggerImage {
    private static final String TRIGGERTYPE = "TRIGGERTYPE";
    private static final String OUTPUT_FOLDER = "triggers";
    private static final String INPUT_FILE_NAME = "triggers.txt";
    private static final String TOP_MESSAGE = StartupFrame.LINK_TEXT;
    private static final String DEFAULT_WORK_FOLDER = ".." + File.separator + "unit_tests";

    private static final int WHEEL_BORDER = 20;
    private static final int WHEEL_DIAMETER = 300;
    private static final int SMALL_DIAMETER = 250;

    /**
     * number of extra frames
     */
    public static int EXTRA_COUNT = 1;

    private static String getTriggerName(TriggerWheelInfo triggerName) {
        switch (triggerName.id) {
            case Fields.TT_TT_MAZDA_MIATA_NA:
                return "Miata NA";
            case Fields.TT_TT_MAZDA_MIATA_NB1:
                return "Miata NB";
            case Fields.TT_TT_SUBARU_SVX:
                return "Subaru SVX";
            case Fields.TT_TT_SUBARU_7_6:
                return "Subaru 7/6";
            case Fields.TT_TT_GM_LS_24:
                return "GM 24x";
            case Fields.TT_TT_GM_7X:
                return "GM 7x";
            case Fields.TT_TT_ONE:
                return "Single Tooth";
            case Fields.TT_TT_2JZ_1_12:
                return "2JZ 1/12";
            case Fields.TT_TT_JEEP_18_2_2_2:
                return "18/2/2/2";
            case Fields.TT_TT_RENIX_44_2_2:
                return "44/2/2";
            case Fields.TT_TT_RENIX_66_2_2_2:
                return "66/2/2/2";
        }
        return triggerName.triggerName;
    }

    public static void main(String[] args) throws InvocationTargetException, InterruptedException {
        final String workingFolder;
        if (args.length != 1) {
            workingFolder = DEFAULT_WORK_FOLDER;
        } else {
            workingFolder = args[0];
        }

        FrameHelper f = new FrameHelper();

        JPanel content = new JPanel(new BorderLayout());
        final TriggerPanel triggerPanel = new TriggerPanel() {
            @Override
            public Dimension getPreferredSize() {
                return new Dimension((1 + EXTRA_COUNT) * WIDTH, 480);
            }
        };

        JPanel topPanel = new JPanel(new FlowLayout());
        content.add(topPanel, BorderLayout.NORTH);
        content.add(triggerPanel, BorderLayout.CENTER);

        f.showFrame(content);
        f.getFrame().setSize(900, 700);

        SwingUtilities.invokeAndWait(new Runnable() {
            @Override
            public void run() {
                try {
                    generateImages(workingFolder, triggerPanel, topPanel, content);
                } catch (IOException e) {
                    throw new IllegalStateException(e);
                }
            }
        });
//        System.exit(-1);
    }

    private static void generateImages(String workingFolder, TriggerPanel trigger, JPanel topPanel, JPanel content) throws IOException {
        String fileName = workingFolder + File.separator + INPUT_FILE_NAME;
        BufferedReader br = new BufferedReader(new FileReader(fileName));

        System.out.println("Reading " + fileName);
        String line;
        while ((line = br.readLine()) != null) {
            if (line.trim().startsWith("#")) {
                // skipping a comment
                continue;
            }

            if (line.startsWith(TRIGGERTYPE)) {
                readTrigger(br, line, trigger, topPanel, content);
            }
        }
    }

    private static void readTrigger(BufferedReader reader, String line, TriggerPanel triggerPanel, JPanel topPanel, JPanel content) throws IOException {
        TriggerWheelInfo triggerWheelInfo = TriggerWheelInfo.readTriggerWheelInfo(line, reader);
//        if (triggerWheelInfo.id != Fields.TT_TT_SUBARU_7_6)
//            return;


        topPanel.removeAll();

        JPanel firstWheelControl = createWheelPanel(triggerWheelInfo.getFirstWheeTriggerSignals());

        topPanel.add(firstWheelControl);
        topPanel.add(StartupFrame.createLogoLabel());

        if (!triggerWheelInfo.waves.get(1).list.isEmpty()) {
            JPanel secondWheelControl = createWheelPanel(triggerWheelInfo.getSecondWheeTriggerSignals());
            topPanel.add(secondWheelControl);
        }

        UiUtils.trueLayout(topPanel);
        UiUtils.trueLayout(content);

        triggerPanel.tdcPosition = triggerWheelInfo.tdcPosition;
        List<WaveState> waves = triggerWheelInfo.waves;

        EngineReport re0 = new EngineReport(waves.get(0).list, 720, 720 * (1 + EXTRA_COUNT));
        System.out.println(re0);
        EngineReport re1 = new EngineReport(waves.get(1).list, 720, 720 * (1 + EXTRA_COUNT));
        EngineReport re2 = new EngineReport(waves.get(2).list, 720, 720 * (1 + EXTRA_COUNT));

        triggerPanel.removeAll();
        UpDownImage upDownImage0 = new UpDownImage(re0, "trigger");
        upDownImage0.showMouseOverText = false;
        triggerPanel.add(upDownImage0);

        UpDownImage upDownImage1 = new UpDownImage(re1, "trigger");
        upDownImage1.showMouseOverText = false;

        UpDownImage upDownImage2 = new UpDownImage(re2, "trigger");
        upDownImage2.showMouseOverText = false;

        boolean isSingleSensor = re1.getList().isEmpty();
        boolean isThirdVisible = !re2.getList().isEmpty();

        int height;
        if (isSingleSensor) {
            height = 1;
        } else if (isThirdVisible) {
            height = 3;
        } else {
            height = 2;
        }

        triggerPanel.setLayout(new GridLayout(height, 1));

        if (!isSingleSensor)
            triggerPanel.add(upDownImage1);
        if (isThirdVisible)
            triggerPanel.add(upDownImage2);

        triggerPanel.name = getTriggerName(triggerWheelInfo);
//        triggerPanel.id = "#" + triggerWheelInfo.id;

        UiUtils.trueLayout(triggerPanel);
        UiUtils.trueRepaint(triggerPanel);
        new File(OUTPUT_FOLDER).mkdir();
        UiUtils.saveImage(OUTPUT_FOLDER + File.separator + "trigger_" + triggerWheelInfo.id + ".png", content);
    }

    @NotNull
    private static JPanel createWheelPanel(List<TriggerSignal> wheel) {
        JPanel clock = new JPanel() {
            @Override
            public void paint(Graphics g) {
                super.paint(g);
                g.setColor(Color.black);

                for (int i = 0; i < wheel.size(); i++) {
                    TriggerSignal current = wheel.get(i);

                    drawRadialLine(g, current.angle);

                    double nextAngle = i == wheel.size() - 1 ? 360 + wheel.get(0).angle : wheel.get(i + 1).angle;
                    int arcDuration = (int) (nextAngle - current.angle);
                    if (current.state == 0) {
                        g.drawArc(WHEEL_BORDER, WHEEL_BORDER, WHEEL_DIAMETER, WHEEL_DIAMETER, (int) current.angle, arcDuration);
                    } else {
                        int corner = WHEEL_BORDER + (WHEEL_DIAMETER - SMALL_DIAMETER) / 2;
                        g.drawArc(corner, corner, SMALL_DIAMETER, SMALL_DIAMETER, (int) current.angle, arcDuration);
                    }
                }
            }

            @Override
            public Dimension getPreferredSize() {
                return new Dimension(WHEEL_DIAMETER + 2 * WHEEL_BORDER, WHEEL_DIAMETER + 2 * WHEEL_BORDER);
            }
        };
//        clock.setBackground(Color.orange);
        return clock;
    }

    private static void drawRadialLine(Graphics g, double angle) {
        int center = WHEEL_BORDER + WHEEL_DIAMETER / 2;

        // converting to 'drawArc' angle convention
        angle = 90 + angle;
        double radianAngle = Math.toRadians(angle);

        int smallX = (int) (SMALL_DIAMETER / 2 * Math.sin(radianAngle));
        int smallY = (int) (SMALL_DIAMETER / 2 * Math.cos(radianAngle));
        int largeX = (int) (WHEEL_DIAMETER / 2 * Math.sin(radianAngle));
        int largeY = (int) (WHEEL_DIAMETER / 2 * Math.cos(radianAngle));

        g.drawLine(center + smallX, center + smallY, center + largeX, center + largeY);
    }

    @NotNull
    static List<WaveState> convertSignalsToWaves(List<TriggerSignal> signals) {
        /**
         * todo: what does this code do? does this work?
         * looks to be repeating trigger share couple of times? but not visible on images somehow?
         */
        List<TriggerSignal> toShow = new ArrayList<>(signals);
        for (int i = 1; i <= 2 + EXTRA_COUNT; i++) {
            for (TriggerSignal s : signals)
                toShow.add(new TriggerSignal(s.waveIndex, s.state, s.angle + i * 720));
        }

        List<WaveState> waves = new ArrayList<>();
        waves.add(new WaveState());
        waves.add(new WaveState());
        waves.add(new WaveState());

        for (TriggerSignal s : toShow) {
            WaveState.trigger_value_e signal = (s.state == 0) ? WaveState.trigger_value_e.TV_LOW : WaveState.trigger_value_e.TV_HIGH;

            WaveState waveState = waves.get(s.waveIndex);
            waveState.handle(signal, s.angle);
        }
        for (WaveState wave : waves)
            wave.wrap();

        return waves;
    }

    @NotNull
    static List<TriggerSignal> readSignals(BufferedReader reader, int count) throws IOException {
        String line;
        String[] tokens;
        List<TriggerSignal> signals = new ArrayList<>();

        int index = 0;
        while (index < count) {
            line = reader.readLine();
            if (line.trim().startsWith("#"))
                continue;
            tokens = line.split(" ");
            if (tokens.length < 4)
                throw new IllegalStateException("Unexpected [" + line + "]");
            int signalIndex = Integer.parseInt(tokens[2]);
            int signalState = Integer.parseInt(tokens[3]);
            double angle = Double.parseDouble(tokens[4]);

            TriggerSignal s = new TriggerSignal(signalIndex, signalState, angle);
//            System.out.println(s);
            signals.add(s);
            index++;
        }
        return signals;
    }

    public static int angleToTime(double prevUp) {
        return (int) (prevUp);
    }

    private static class TriggerPanel extends JPanel {
        public String name = "";
        public String id;
        public double tdcPosition;

        @Override
        public void paint(Graphics g) {
            super.paint(g);

            g.setColor(Color.black);

            int w = getWidth();

            int off = g.getFontMetrics().stringWidth(TOP_MESSAGE);
            g.drawString(TOP_MESSAGE, w - off, g.getFont().getSize());

            String line = new Date().toString();
            off = g.getFontMetrics().stringWidth(line);
            g.drawString(line, w - off, 2 * g.getFont().getSize());

            Font f = g.getFont();
            g.setFont(new Font(f.getName(), Font.BOLD, f.getSize() * 3));

            int h = getHeight();

            g.drawString(name, 0, (int) (h * 0.75));
            if (id != null)
                g.drawString(id, 0, (int) (h * 0.9));

            g.setColor(Color.green);
            int tdcFontSize = (int) (f.getSize() * 1.5);
            g.setFont(new Font(f.getName(), Font.BOLD, tdcFontSize));
            g.drawString("tdcPosition " + formatTdcPosition(), 0, tdcFontSize);

            int tdcX = (int) (w / 720.0 * tdcPosition);
            g.drawLine(tdcX, 0, tdcX, h);
        }

        private String formatTdcPosition() {
            if ((int) tdcPosition == tdcPosition)
                return Integer.toString((int) tdcPosition);
            return Double.toString(tdcPosition);
        }
    }
}