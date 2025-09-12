document.addEventListener('DOMContentLoaded', () => {
    // --- DOM Elements ---
    const lcdRow1 = document.getElementById('lcd-row1');
    const lcdRow2 = document.getElementById('lcd-row2');
    const btnSelect = document.getElementById('btn-select');
    const btnLeft = document.getElementById('btn-left');
    const btnUp = document.getElementById('btn-up');
    const btnDown = document.getElementById('btn-down');
    const btnRight = document.getElementById('btn-right');
    const shutterIndicator = document.getElementById('shutter-indicator');

    // --- Constants ---
    const NONE = 0;
    const SELECT = 1;
    const LEFT = 2;
    const UP = 3;
    const DOWN = 4;
    const RIGHT = 5;

    const SCR_INTERVAL = 0;
    const SCR_SHOTS = 1;
    const SCR_RUNNING = 2;
    const SCR_CONFIRM_END = 3;
    const SCR_SETTINGS = 4;
    const SCR_PAUSE = 5;
    const SCR_RAMP_TIME = 6;
    const SCR_RAMP_TO = 7;
    const SCR_DONE = 8;
    const SCR_MODE = 9;
    const SCR_EXPOSURE = 10;
    const SCR_SINGLE = 11;
    const SCR_CONFIRM_END_BULB = 12;


    const MODE_M = 0;
    const MODE_BULB = 1;

    const CAPTION = "Pro-Timer 0.92";
    const RELEASE_TIME_DEFAULT = 0.1;
    const MIN_DARK_TIME = 0.5;
    const cMinInterval = 0.2;
    const cMaxInterval = 999;

    // --- State ---
    let state = {
        currentMenu: -1, // Welcome screen
        interval: 4.0,
        maxNoOfShots: 0,
        isRunning: 0,
        imageCount: 0,
        runningTime: 0,
        releaseTime: RELEASE_TIME_DEFAULT,
        mode: MODE_M,
        bulbReleasedAt: 0,
        rampDuration: 10,
        rampTo: 4.0,
        rampingStartTime: 0,
        rampingEndTime: 0,
        intervalBeforeRamping: 0,
        previousMillis: 0,
        settingsSel: 1,
    };

    // --- Timing ---
    let startTime = performance.now();
    const millis = () => performance.now() - startTime;

    // --- Rendering Helpers ---
    function clearLCD() {
        lcdRow1.textContent = '';
        lcdRow2.textContent = '';
    }

    function printToLCD(row, text) {
        const el = row === 1 ? lcdRow1 : lcdRow2;
        el.textContent = text.toString().padEnd(16, ' ');
    }

    function printFloat(f, total, dec) {
        return f.toFixed(dec).padStart(total, ' ');
    }

    function printInt(i, total) {
        return i.toString().padStart(total, ' ');
    }

    function fillZero(num) {
        return num.toString().padStart(2, '0');
    }


    // --- Screen Printing Functions ---

    function printWelcomeScreen() {
        printToLCD(1, "LRTimelapse.com");
        printToLCD(2, CAPTION);
    }

    function printIntervalMenu() {
        printToLCD(1, "Interval");
        let intervalStr = state.interval < 20 ? printFloat(state.interval, 5, 1) : printFloat(state.interval, 3, 0);
        printToLCD(2, intervalStr);
    }

    function printModeMenu() {
        printToLCD(1, "Mode");
        if (state.mode === MODE_M) {
            printToLCD(2, "M (Default)");
        } else {
            printToLCD(2, "Bulb (Astro)");
        }
    }

    function printNoOfShotsMenu() {
        printToLCD(1, "No of shots");
        if (state.maxNoOfShots > 0) {
            printToLCD(2, printInt(state.maxNoOfShots, 4));
        } else {
            printToLCD(2, "unlimited");
        }
    }

    function printExposureMenu() {
        printToLCD(1, "Exposure");
        printToLCD(2, state.releaseTime.toFixed(1));
    }

    function printRunningScreen() {
        let line1 = printInt(state.imageCount, 4);
        if (state.maxNoOfShots > 0) {
            line1 += " R:" + printInt(state.maxNoOfShots - state.imageCount, 4);
        }
        printToLCD(1, line1);

        let line2 = "";
        if (state.maxNoOfShots > 0) {
            const remainingSecs = (state.maxNoOfShots - state.imageCount) * state.interval;
            line2 += "T-" + fillZero(Math.floor(remainingSecs / 3600)) + ":" + fillZero(Math.floor((remainingSecs / 60) % 60));
        }
        printToLCD(2, line2);
        updateTime(); // This will overwrite part of the line
    }

    function updateTime() {
        if (!state.isRunning) {
            printToLCD(2, "        Done!");
            return;
        }
        const finerRunningTime = state.runningTime + (millis() - state.previousMillis);
        const hours = Math.floor(finerRunningTime / 1000 / 3600);
        const minutes = Math.floor((finerRunningTime / 1000 / 60) % 60);
        const secs = Math.floor((finerRunningTime / 1000) % 60);

        const timeStr = `        ${fillZero(hours)}:${fillZero(minutes)}:${fillZero(secs)}`;
        lcdRow2.textContent = lcdRow2.textContent.substring(0, 8) + timeStr.substring(8);

        let intervalStr = millis() < state.rampingEndTime ? "*" : " ";
        intervalStr += state.interval < 100 ? printFloat(state.interval, 4, 1) : printFloat(state.interval, 4, 0);
        lcdRow1.textContent = lcdRow1.textContent.substring(0, 11) + intervalStr;
    }


    function printDoneScreen() {
        printToLCD(1, `Done ${state.imageCount} shots.`);
        const hours = Math.floor(state.runningTime / 1000 / 3600);
        const minutes = Math.floor((state.runningTime / 1000 / 60) % 60);
        const timeStr = `t=${fillZero(hours)}:${fillZero(minutes)}    ;-)`;
        printToLCD(2, timeStr);
    }

    function printConfirmEndScreen() {
        printToLCD(1, "Stop shooting?");
        printToLCD(2, "< Stop    Cont.>");
    }

    function printConfirmEndScreenBulb() {
        printToLCD(1, "Stop exposure?");
        printToLCD(2, "< Cont.   Stop >");
    }

    function printSettingsMenu() {
        printToLCD(1, ">Pause");
        printToLCD(2, " Ramp Interval");
        if (state.settingsSel === 2) {
            printToLCD(1, " Pause");
            printToLCD(2, ">Ramp Interval");
        }
    }

    function printPauseMenu() {
        printToLCD(1, "PAUSE...");
        printToLCD(2, "< Continue");
    }

    function printRampDurationMenu() {
        printToLCD(1, "Ramp Time (min)");
        printToLCD(2, state.rampDuration);
    }

    function printRampToMenu() {
        printToLCD(1, "Ramp to (Intvl.)");
        const rampToStr = state.rampTo < 20 ? printFloat(state.rampTo, 5, 1) : printFloat(state.rampTo, 3, 0);
        printToLCD(2, rampToStr);
    }

    function printSingleScreen() {
        if (state.releaseTime < 1) {
            printToLCD(1, "Single Exposure");
            printToLCD(2, state.releaseTime.toFixed(1) + "       < FIRE");
        } else {
            printToLCD(1, "Bulb Exposure");
            if (state.bulbReleasedAt === 0) {
                const hours = Math.floor(state.releaseTime / 3600);
                const minutes = Math.floor((state.releaseTime / 60) % 60);
                const secs = Math.floor(state.releaseTime % 60);
                printToLCD(2, `${fillZero(hours)}:${fillZero(minutes)}'${fillZero(secs)}" < FIRE`);
            } else {
                 const runningTime = (state.bulbReleasedAt + state.releaseTime * 1000) - millis();
                 const hours = Math.floor(runningTime / 1000 / 3600);
                 const minutes = Math.floor((runningTime / 1000 / 60) % 60);
                 const secs = Math.floor((runningTime / 1000) % 60);
                 printToLCD(2, `        ${fillZero(hours)}:${fillZero(minutes)}:${fillZero(secs)}`);
            }
        }
    }


    function printScreen() {
        clearLCD();
        switch (state.currentMenu) {
            case SCR_INTERVAL: printIntervalMenu(); break;
            case SCR_MODE: printModeMenu(); break;
            case SCR_SHOTS: printNoOfShotsMenu(); break;
            case SCR_EXPOSURE: printExposureMenu(); break;
            case SCR_RUNNING: printRunningScreen(); break;
            case SCR_CONFIRM_END: printConfirmEndScreen(); break;
            case SCR_CONFIRM_END_BULB: printConfirmEndScreenBulb(); break;
            case SCR_SETTINGS: printSettingsMenu(); break;
            case SCR_PAUSE: printPauseMenu(); break;
            case SCR_RAMP_TIME: printRampDurationMenu(); break;
            case SCR_RAMP_TO: printRampToMenu(); break;
            case SCR_DONE: printDoneScreen(); break;
            case SCR_SINGLE: printSingleScreen(); break;
        }
    }

    // --- Core Logic ---

    function releaseCamera() {
        console.log("Shutter triggered!");
        shutterIndicator.classList.add('triggered');
        setTimeout(() => {
            shutterIndicator.classList.remove('triggered');
        }, 200); // Flash for 200ms

        if (state.releaseTime >= 1) { // Bulb mode
            if (state.bulbReleasedAt === 0) {
                state.bulbReleasedAt = millis();
            }
        }
    }

    function stopShooting() {
        state.isRunning = 0;
        state.imageCount = 0;
        state.runningTime = 0;
        state.bulbReleasedAt = 0;
    }

    function possiblyEndLongExposure() {
        if (state.bulbReleasedAt !== 0 && (millis() >= (state.bulbReleasedAt + state.releaseTime * 1000))) {
            state.bulbReleasedAt = 0;
        }
        if (state.currentMenu === SCR_SINGLE || state.currentMenu === SCR_RUNNING) {
            printScreen();
        }
    }

    function possiblyRampInterval() {
        if (millis() < state.rampingEndTime && millis() >= state.rampingStartTime) {
            const elapsed = millis() - state.rampingStartTime;
            const duration = state.rampingEndTime - state.rampingStartTime;
            state.interval = state.intervalBeforeRamping + (elapsed / duration) * (state.rampTo - state.intervalBeforeRamping);

            if (state.releaseTime > state.interval - MIN_DARK_TIME) {
                state.releaseTime = state.interval - MIN_DARK_TIME;
            }
        } else {
            state.rampingStartTime = 0;
            state.rampingEndTime = 0;
        }
    }

    function running() {
        if (!state.isRunning) return;

        if (millis() - state.previousMillis >= state.interval * 1000) {
            if (state.maxNoOfShots !== 0 && state.imageCount >= state.maxNoOfShots) {
                state.isRunning = 0;
                state.currentMenu = SCR_DONE;
                printScreen();
                stopShooting();
            } else {
                state.runningTime += (millis() - state.previousMillis);
                state.previousMillis = millis();
                releaseCamera();
                state.imageCount++;
            }
        }
        possiblyRampInterval();
    }


    function processKey(key) {
        if (state.currentMenu === -1 && key) { // Any key press to exit welcome
             state.currentMenu = SCR_INTERVAL;
             printScreen();
             return;
        }

        switch (state.currentMenu) {
            case SCR_INTERVAL:
                if (key === UP) {
                    state.interval = state.interval < 20 ? parseFloat((state.interval + 0.1).toFixed(1)) : state.interval + 1;
                    if (state.interval > cMaxInterval) state.interval = cMaxInterval;
                } else if (key === DOWN) {
                    state.interval = state.interval > cMinInterval ? (state.interval < 20 ? parseFloat((state.interval - 0.1).toFixed(1)) : state.interval - 1) : cMinInterval;
                } else if (key === RIGHT) {
                    state.rampTo = state.interval;
                    state.currentMenu = SCR_MODE;
                } else if (key === LEFT) {
                    state.currentMenu = SCR_SINGLE;
                }
                break;

            case SCR_MODE:
                if (key === RIGHT) state.currentMenu = SCR_SHOTS;
                else if (key === LEFT) state.currentMenu = SCR_INTERVAL;
                else if (key === UP || key === DOWN) {
                    state.mode = state.mode === MODE_M ? MODE_BULB : MODE_M;
                    if (state.mode === MODE_M) state.releaseTime = RELEASE_TIME_DEFAULT;
                }
                break;

            case SCR_SHOTS:
                if (key === UP) {
                    if (state.maxNoOfShots >= 2500) state.maxNoOfShots += 100;
                    else if (state.maxNoOfShots >= 1000) state.maxNoOfShots += 50;
                    else if (state.maxNoOfShots >= 100) state.maxNoOfShots += 25;
                    else if (state.maxNoOfShots >= 10) state.maxNoOfShots += 10;
                    else state.maxNoOfShots++;
                    if (state.maxNoOfShots > 9999) state.maxNoOfShots = 9999;
                } else if (key === DOWN) {
                    if (state.maxNoOfShots > 2500) state.maxNoOfShots -= 100;
                    else if (state.maxNoOfShots > 1000) state.maxNoOfShots -= 50;
                    else if (state.maxNoOfShots > 100) state.maxNoOfShots -= 25;
                    else if (state.maxNoOfShots > 10) state.maxNoOfShots -= 10;
                    else if (state.maxNoOfShots > 0) state.maxNoOfShots--;
                } else if (key === LEFT) {
                    state.currentMenu = SCR_MODE;
                } else if (key === RIGHT) {
                    if (state.mode === MODE_M) {
                        state.currentMenu = SCR_RUNNING;
                        state.previousMillis = millis();
                        state.runningTime = 0;
                        state.isRunning = 1;
                        releaseCamera();
                        state.imageCount++;
                    } else {
                        state.currentMenu = SCR_EXPOSURE;
                    }
                }
                break;

            case SCR_EXPOSURE:
                if (key === UP) {
                    state.releaseTime = parseFloat((state.releaseTime + 0.1).toFixed(1));
                    if (state.releaseTime > state.interval - MIN_DARK_TIME) {
                        state.releaseTime = state.interval - MIN_DARK_TIME;
                    }
                } else if (key === DOWN) {
                    if (state.releaseTime > RELEASE_TIME_DEFAULT) {
                        state.releaseTime = parseFloat((state.releaseTime - 0.1).toFixed(1));
                    }
                } else if (key === LEFT) {
                    state.currentMenu = SCR_SHOTS;
                } else if (key === RIGHT) {
                    state.currentMenu = SCR_RUNNING;
                    state.previousMillis = millis();
                    state.runningTime = 0;
                    state.isRunning = 1;
                    releaseCamera();
                    state.imageCount++;
                }
                break;

            case SCR_RUNNING:
                if (key === LEFT) {
                    if (state.rampingEndTime === 0) {
                        state.currentMenu = SCR_CONFIRM_END;
                    } else {
                        state.rampingStartTime = 0;
                        state.rampingEndTime = 0;
                    }
                } else if (key === RIGHT) {
                    state.currentMenu = SCR_SETTINGS;
                }
                break;

            case SCR_CONFIRM_END:
                if (key === LEFT) { // Stop
                    if (state.bulbReleasedAt > 0) state.bulbReleasedAt = 0;
                    stopShooting();
                    state.currentMenu = SCR_INTERVAL;
                } else if (key === RIGHT) { // Continue
                    state.currentMenu = SCR_RUNNING;
                }
                break;

            case SCR_SETTINGS:
                if (key === UP && state.settingsSel === 2) state.settingsSel = 1;
                else if (key === DOWN && state.settingsSel === 1) state.settingsSel = 2;
                else if (key === LEFT) state.currentMenu = SCR_RUNNING;
                else if (key === RIGHT) {
                    if (state.settingsSel === 1) { // Pause
                        state.isRunning = 0;
                        state.currentMenu = SCR_PAUSE;
                    } else { // Ramp
                        state.currentMenu = SCR_RAMP_TIME;
                    }
                }
                break;

            case SCR_PAUSE:
                if (key === LEFT) { // Continue
                    state.isRunning = 1;
                    state.previousMillis = millis() - state.runningTime; // Adjust start time
                    state.currentMenu = SCR_RUNNING;
                }
                break;

            case SCR_RAMP_TIME:
                if (key === UP) state.rampDuration = state.rampDuration >= 10 ? state.rampDuration + 10 : state.rampDuration + 1;
                else if (key === DOWN) {
                    state.rampDuration = state.rampDuration > 10 ? state.rampDuration - 10 : state.rampDuration - 1;
                    if (state.rampDuration < 1) state.rampDuration = 1;
                }
                else if (key === LEFT) state.currentMenu = SCR_SETTINGS;
                else if (key === RIGHT) state.currentMenu = SCR_RAMP_TO;
                break;

            case SCR_RAMP_TO:
                 if (key === UP) {
                    state.rampTo = state.rampTo < 20 ? parseFloat((state.rampTo + 0.1).toFixed(1)) : state.rampTo + 1;
                    if (state.rampTo > cMaxInterval) state.rampTo = cMaxInterval;
                } else if (key === DOWN) {
                    state.rampTo = state.rampTo > cMinInterval ? (state.rampTo < 20 ? parseFloat((state.rampTo - 0.1).toFixed(1)) : state.rampTo - 1) : cMinInterval;
                } else if (key === LEFT) {
                    state.currentMenu = SCR_RAMP_TIME;
                } else if (key === RIGHT) {
                    if (state.rampTo !== state.interval) {
                        state.intervalBeforeRamping = state.interval;
                        state.rampingStartTime = millis();
                        state.rampingEndTime = state.rampingStartTime + state.rampDuration * 60 * 1000;
                    }
                    state.currentMenu = SCR_RUNNING;
                }
                break;

            case SCR_DONE:
                if (key === LEFT || key === RIGHT) {
                    stopShooting();
                    state.currentMenu = SCR_INTERVAL;
                }
                break;

            case SCR_SINGLE:
                if (key === UP) {
                    state.releaseTime = state.releaseTime < 60 ? state.releaseTime + 1 : state.releaseTime + 10;
                } else if (key === DOWN) {
                    state.releaseTime = state.releaseTime > RELEASE_TIME_DEFAULT ? (state.releaseTime < 60 ? state.releaseTime - 1 : state.releaseTime - 10) : RELEASE_TIME_DEFAULT;
                } else if (key === LEFT) { // FIRE
                    releaseCamera();
                } else if (key === RIGHT) {
                    state.currentMenu = state.bulbReleasedAt === 0 ? SCR_INTERVAL : SCR_CONFIRM_END_BULB;
                }
                break;

            case SCR_CONFIRM_END_BULB:
                if (key === RIGHT) { // Stop
                    stopShooting();
                    state.currentMenu = SCR_SINGLE;
                } else if (key === LEFT) { // Continue
                    state.currentMenu = SCR_SINGLE;
                }
                break;
        }

        printScreen();
    }


    // --- Main ---
    function main() {
        printWelcomeScreen();

        setTimeout(() => {
            if (state.currentMenu === -1) { // If user hasn't pressed a key
                state.currentMenu = SCR_INTERVAL;
                printScreen();
            }
        }, 2000);

        setInterval(() => {
            if (state.isRunning) {
                running();
                printRunningScreen();
            }
            if (state.bulbReleasedAt > 0 || (state.currentMenu === SCR_SINGLE && state.releaseTime >=1)) {
                possiblyEndLongExposure();
            }
        }, 100);

        // --- Event Listeners ---
        btnSelect.addEventListener('click', () => processKey(SELECT));
        btnLeft.addEventListener('click', () => processKey(LEFT));
        btnUp.addEventListener('click', () => processKey(UP));
        btnDown.addEventListener('click', () => processKey(DOWN));
        btnRight.addEventListener('click', () => processKey(RIGHT));

        document.addEventListener('keydown', (e) => {
            switch (e.key) {
                case 'ArrowUp': processKey(UP); break;
                case 'ArrowDown': processKey(DOWN); break;
                case 'ArrowLeft': processKey(LEFT); break;
                case 'ArrowRight': processKey(RIGHT); break;
                case 'Enter': case ' ': processKey(SELECT); break;
            }
        });
    }

    main();
});
