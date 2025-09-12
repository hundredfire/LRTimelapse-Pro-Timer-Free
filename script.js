// Constants for keys
const NONE = 0;
const SELECT = 1;
const LEFT = 2;
const UP = 3;
const DOWN = 4;
const RIGHT = 5;

// Menu workflow constants
const SCR_INTERVAL = 0;
const SCR_SHOTS = 1;
const SCR_MODE = 9;
const SCR_EXPOSURE = 10;
const SCR_RUNNING = 2;
const SCR_CONFIRM_END = 3;
const SCR_SETTINGS = 4;
const SCR_PAUSE = 5;
const SCR_RAMP_TIME = 6;
const SCR_RAMP_TO = 7;
const SCR_DONE = 8;
const SCR_SINGLE = 11;
const SCR_CONFIRM_END_BULB = 12;

// Mode constants
const MODE_M = 0;
const MODE_BULB = 1;
const MODE_SINGLE = 3;
const MODE_SETUP = 2;


// State variables
let currentMenu = SCR_MODE;
let interval = 2.0;
let maxNoOfShots = 0;
let isRunning = 0;
let imageCount = 0;
let releaseTime = 0.1;
let bulbReleasedAt = 0;
let runningTime = 0;
let previousMillis = 0;
let rampDuration = 10;
let rampTo = 0.0;
let rampingStartTime = 0;
let rampingEndTime = 0;
let intervalBeforeRamping = 0;
let mode = MODE_M;
let settingsSel = 1;

let runningInterval;

// DOM Elements
const lcdRow1 = document.getElementById('lcd-row-1');
const lcdRow2 = document.getElementById('lcd-row-2');
const cameraIndicator = document.getElementById('camera-indicator');

// --- Main Logic ---

function processKey(key) {
    switch (currentMenu) {
        case SCR_MODE:
            if (key === UP) {
                mode = (mode + 1) % 4;
            } else if (key === DOWN) {
                mode = (mode - 1 + 4) % 4;
            } else if (key === RIGHT) {
                if(mode === MODE_SINGLE){
                    currentMenu = SCR_SINGLE;
                } else {
                    currentMenu = SCR_INTERVAL;
                }
            }
            break;

        case SCR_INTERVAL:
            if (key === UP) {
                interval = parseFloat((interval + 0.1).toFixed(1));
            } else if (key === DOWN) {
                interval = parseFloat((interval - 0.1).toFixed(1));
                if (interval < 0.2) interval = 0.2;
            } else if (key === RIGHT) {
                currentMenu = SCR_SHOTS;
            } else if (key === LEFT) {
                currentMenu = SCR_MODE;
            }
            break;

        case SCR_SHOTS:
            if (key === UP) {
                maxNoOfShots++;
            } else if (key === DOWN) {
                maxNoOfShots--;
                if (maxNoOfShots < 0) maxNoOfShots = 0;
            } else if (key === RIGHT) {
                if (mode === MODE_M || mode === MODE_BULB) {
                     if (mode === MODE_BULB) {
                        currentMenu = SCR_EXPOSURE;
                    } else {
                        startShooting();
                    }
                }
            } else if (key === LEFT) {
                currentMenu = SCR_INTERVAL;
            }
            break;

        case SCR_EXPOSURE:
            if (key === UP) {
                releaseTime = parseFloat((releaseTime + 0.1).toFixed(1));
            } else if (key === DOWN) {
                releaseTime = parseFloat((releaseTime - 0.1).toFixed(1));
                if (releaseTime < 0.1) releaseTime = 0.1;
            } else if (key === RIGHT) {
                startShooting();
            } else if (key === LEFT) {
                currentMenu = SCR_SHOTS;
            }
            break;

        case SCR_SINGLE:
            if (key === RIGHT) {
                releaseCamera();
            } else if (key === LEFT) {
                currentMenu = SCR_MODE;
            }
            break;

        case SCR_RUNNING:
            if (key === LEFT) {
                currentMenu = SCR_CONFIRM_END;
            }
            break;

        case SCR_CONFIRM_END:
            if (key === LEFT) {
                stopShooting();
                currentMenu = SCR_MODE;
            } else if (key === RIGHT) {
                currentMenu = SCR_RUNNING;
            }
            break;

        case SCR_DONE:
            if (key === LEFT || key === RIGHT) {
                currentMenu = SCR_MODE;
                imageCount = 0;
            }
            break;
    }
    printScreen();
}

function startShooting() {
    isRunning = 1;
    imageCount = 0;
    runningTime = 0;
    previousMillis = Date.now();
    currentMenu = SCR_RUNNING;
    releaseCamera();
    runningInterval = setInterval(running, interval * 1000);
}

function stopShooting() {
    isRunning = 0;
    clearInterval(runningInterval);
}

function running() {
    if (!isRunning) return;

    if (maxNoOfShots > 0 && imageCount >= maxNoOfShots) {
        stopShooting();
        currentMenu = SCR_DONE;
        printScreen();
        return;
    }

    releaseCamera();
    runningTime += Date.now() - previousMillis;
    previousMillis = Date.now();

    printScreen();
}

function printScreen() {
    lcdRow1.innerHTML = '';
    lcdRow2.innerHTML = '';

    switch (currentMenu) {
        case SCR_MODE:
            printModeMenu();
            break;
        case SCR_INTERVAL:
            printIntervalMenu();
            break;
        case SCR_SHOTS:
            printNoOfShotsMenu();
            break;
        case SCR_EXPOSURE:
            printExposureMenu();
            break;
        case SCR_SINGLE:
            printSingleScreen();
            break;
        case SCR_RUNNING:
            printRunningScreen();
            break;
        case SCR_CONFIRM_END:
            printConfirmEndScreen();
            break;
        case SCR_DONE:
            printDoneScreen();
            break;
        default:
            lcdRow1.textContent = "Not Implemented";
            lcdRow2.textContent = "Menu: " + currentMenu;
    }
}

function releaseCamera() {
    console.log("Camera triggered!");
    cameraIndicator.style.backgroundColor = getRandomColor();
    setTimeout(() => {
        cameraIndicator.style.backgroundColor = '#ccc';
    }, 200);
    imageCount++;
}

// --- Print Functions ---

function printModeMenu() {
    lcdRow1.textContent = "Mode";
    switch (mode) {
        case MODE_M:
            lcdRow2.textContent = "Timelapse (M)";
            break;
        case MODE_BULB:
            lcdRow2.textContent = "TL Bulb (Astro)";
            break;
        case MODE_SINGLE:
            lcdRow2.textContent = "Single Exposure";
            break;
        case MODE_SETUP:
            lcdRow2.textContent = "Setup";
            break;
    }
}

function printIntervalMenu() {
    lcdRow1.textContent = "Interval";
    lcdRow2.textContent = `${printFloat(interval, 5, 1)} sec`;
}

function printNoOfShotsMenu() {
    lcdRow1.textContent = "No of shots";
    if (maxNoOfShots > 0) {
        lcdRow2.textContent = `${maxNoOfShots}`;
    } else {
        lcdRow2.textContent = "unlimited";
    }
}

function printExposureMenu() {
    lcdRow1.textContent = "Exposure";
    lcdRow2.textContent = `${printFloat(releaseTime, 5, 1)} sec`;
}

function printSingleScreen() {
    lcdRow1.textContent = "Single Exposure";
    lcdRow2.textContent = "Press Right to fire";
}

function printRunningScreen() {
    lcdRow1.textContent = `${imageCount} shots`;
    if (maxNoOfShots > 0) {
        lcdRow1.textContent += ` / ${maxNoOfShots}`;
    }
    const elapsed = (Date.now() - previousMillis + runningTime) / 1000;
    const hours = Math.floor(elapsed / 3600);
    const minutes = Math.floor((elapsed % 3600) / 60);
    const seconds = Math.floor(elapsed % 60);
    lcdRow2.textContent = `Time: ${fillZero(hours)}:${fillZero(minutes)}:${fillZero(seconds)}`;
}

function printConfirmEndScreen() {
    lcdRow1.textContent = "Stop shooting?";
    lcdRow2.textContent = "< Stop    Cont.>";
}

function printDoneScreen() {
    lcdRow1.textContent = `Done ${imageCount} shots.`;
    const elapsed = runningTime / 1000;
    const hours = Math.floor(elapsed / 3600);
    const minutes = Math.floor((elapsed % 3600) / 60);
    const seconds = Math.floor(elapsed % 60);
    lcdRow2.textContent = `t=${fillZero(hours)}:${fillZero(minutes)}:${fillZero(seconds)}  ;-)`;
}


// --- Helper Functions ---

function fillZero(num) {
    return num < 10 ? "0" + num : String(num);
}

function printFloat(num, total, dec) {
    return num.toFixed(dec).padStart(total, ' ');
}

function getRandomColor() {
    const letters = '0123456789ABCDEF';
    let color = '#';
    for (let i = 0; i < 6; i++) {
        color += letters[Math.floor(Math.random() * 16)];
    }
    return color;
}

// --- Initialization ---

function init() {
    printScreen();
    // Add event listeners for keypad
    document.getElementById('key-up').addEventListener('click', () => processKey(UP));
    document.getElementById('key-down').addEventListener('click', () => processKey(DOWN));
    document.getElementById('key-left').addEventListener('click', () => processKey(LEFT));
    document.getElementById('key-right').addEventListener('click', () => processKey(RIGHT));
    document.getElementById('key-select').addEventListener('click', () => processKey(SELECT));
}

// Start the application
init();
