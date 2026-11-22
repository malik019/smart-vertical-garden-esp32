document.addEventListener("DOMContentLoaded", () => {
    // Initial profile configuration pull down from flash filesystem
    fetchSystemConfig();
    
    // Continuous execution telemetry polling loop (Every 2 seconds)
    setInterval(pollLiveTelemetry, 2000);

    // Bind the interactive HTML5 range slider event handler
    const slider = document.getElementById("light-slider");
    if (slider) {
        // Using 'input' instead of 'change' for fluid, real-time updates while dragging
        slider.addEventListener("input", sendSliderOverride);
    }

    // Expose the core module handlers directly to the global window HTML scope
    // This stops the inline onclick/onsubmit attributes from throwing "not defined" errors
    window.sendOverride = sendOverride;
    window.saveSystemConfig = saveSystemConfig;
});

async function pollLiveTelemetry() {
    try {
        const response = await fetch('/api/live');
        if (!response.ok) throw new Error("Telemetry channel breakdown");
        const data = await response.json();

        // Inject active metrics straight into your grid layout elements
        document.getElementById('system-time').innerText = `System Local Time: ${data.time}`;
        document.getElementById('room-temp').innerText = data.room_temp.toFixed(1);
        document.getElementById('room-humidity').innerText = data.room_humidity.toFixed(0);
        document.getElementById('water-temp').innerText = data.water_temp.toFixed(1);
        document.getElementById('water-level').innerText = data.water_level.toFixed(0);
        document.getElementById('pump-state').innerText = data.pump;

        // Calculate and process light dimmer percentage bounds
        const brightnessPct = Math.round((data.light_brightness / 255) * 100);

        // Update the textual brightness display under the knob
        const textElement = document.getElementById('light-brightness-text');
        if (textElement) {
            textElement.innerText = `${brightnessPct}%`;
        }

        // CRITICAL FIX: Only let the automated poll move the slider if the user 
        // is NOT currently holding or active on it (:focus)
        const slider = document.getElementById('light-slider');
        if (slider && !slider.matches(':focus')) {
            slider.value = brightnessPct;
        }

        // Parse alert state metrics and toggle visibility of the crimson warning banner
        const alarmBanner = document.getElementById('alarm-banner');
        if (alarmBanner) {
            if (data.alarm !== "NONE") {
                alarmBanner.innerText = `CRITICAL ALARM STATUS TRIGGERED: ${data.alarm.replace(/_/g, ' ')}`;
                alarmBanner.classList.remove('hidden');
                alarmBanner.classList.add('visible');
            } else {
                alarmBanner.classList.remove('visible');
                alarmBanner.classList.add('hidden');
            }
        }
    } catch (err) {
        console.error("Polling system failure trace:", err);
    }
}

async function fetchSystemConfig() {
    try {
        const response = await fetch('/config');
        const config = await response.json();

        // Prepopulate configuration wizard inputs with active profiles
        document.getElementById('light_start').value = config.lighting.start_time;
        document.getElementById('light_end').value = config.lighting.end_time;
        document.getElementById('fade_duration_min').value = config.lighting.fade_duration_min;
        document.getElementById('pump_on').value = config.irrigation.on_duration_min;
        document.getElementById('pump_interval').value = config.irrigation.interval_min;
        document.getElementById('min_water').value = config.thresholds.min_water_level_pct;
    } catch (err) {
        console.error("Config population error trace:", err);
    }
}

async function sendOverride(state) {
    try {
        await fetch('/api/override', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ pump: state, timeout: 300 }) // 5-minute safety override window
        });
        console.log(`[Pump] Manual directive dispatched successfully: ${state}`);
    } catch (err) {
        console.error("Transmission breakdown passing manual runtime commands:", err);
    }
}

async function sendSliderOverride() {
    const sliderValue = parseInt(document.getElementById("light-slider").value);
    const rawPwm = Math.round((sliderValue / 100) * 255); // Remap 0-100% to a raw 0-255 byte array

    // Immediately update textual readout for ultra-responsive fluid UI feel
    const textElement = document.getElementById('light-brightness-text');
    if (textElement) {
        textElement.innerText = `${sliderValue}%`;
    }

    try {
        // FIXED: Added a dynamic timestamp query string (?ts=...) to force the ESPAsyncWebServer 
        // to bypass the browser's persistent internal networking cache loops
        await fetch(`/api/override?ts=${Date.now()}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ brightness: rawPwm, timeout: 300 })
        });
        console.log(`[Slider] Successfully verified manual override packet: ${sliderValue}% (${rawPwm}/255 PWM)`);
    } catch (err) {
        console.error("Fout bij verzenden dimmerwaarde:", err);
    }
}

async function saveSystemConfig(event) {
    event.preventDefault(); // Lock form from performing a native page redirection fall-through
    const configPayload = {
        light_start: document.getElementById('light_start').value,
        light_end: document.getElementById('light_end').value,
        fade_duration_min: parseInt(document.getElementById('fade_duration_min').value),
        pump_on: parseInt(document.getElementById('pump_on').value),
        pump_interval: parseInt(document.getElementById('pump_interval').value),
        min_water: parseFloat(document.getElementById('min_water').value)
    };

    try {
        const response = await fetch('/api/saveconfig', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(configPayload)
        });
        if (response.ok) {
            alert("System profiles compiled and written securely to Flash storage.");
        }
    } catch (err) {
        alert("Flash filesystem write operation failed.");
    }
}
