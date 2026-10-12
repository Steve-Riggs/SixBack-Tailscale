#include "microlink_bridge.h"

#ifdef SIXBACK_MICROLINK_ENABLED

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "microlink.h"

namespace {

microlink_t* gMicrolink = nullptr;

// Keep this String alive because MicroLink retains a pointer
// to the authentication key after initialization.
String gAuthKey;

void onMicrolinkState(
    microlink_t* ml,
    microlink_state_t state,
    void* userData
) {
    Serial.printf("[tailscale] state=%d\n", (int)state);

    if (state == ML_STATE_CONNECTED) {
        char vpnIp[16] = {};

        microlink_ip_to_str(
            microlink_get_vpn_ip(ml),
            vpnIp
        );

        Serial.printf(
            "[tailscale] connected, VPN IP=%s\n",
            vpnIp
        );
    }
}

} // namespace

namespace sixback {


bool handleMicrolinkSerialCommand(const String& line) {
    if (line == "vpn-status") {
        Preferences prefs;
        bool configured = false;

        if (prefs.begin("sixback-vpn", true)) {
            configured = prefs.isKey("authkey");
            prefs.end();
        }

        Serial.printf(
            "[tailscale] configured=%d state=%d\\n",
            (int)configured,
            gMicrolink ? (int)microlink_get_state(gMicrolink) : -1
        );

        if (gMicrolink && microlink_is_connected(gMicrolink)) {
            char ip[16] = {};
            microlink_ip_to_str(microlink_get_vpn_ip(gMicrolink), ip);
            Serial.printf("[tailscale] VPN IP=%s\\n", ip);
        }
        return true;
    }

    if (line.startsWith("vpn-set ")) {
        String key = line.substring(8);
        key.trim();

        if (!key.startsWith("tskey-auth-") ||
            key.length() < 20 || key.length() > 160) {
            Serial.println("[tailscale] Invalid auth key format");
            return true;
        }

        Preferences prefs;
        if (!prefs.begin("sixback-vpn", false)) {
            Serial.println("[tailscale] Cannot open NVS");
            return true;
        }

        size_t saved = prefs.putString("authkey", key);
        prefs.end();

        if (saved != key.length()) {
            Serial.println("[tailscale] Failed to save auth key");
            return true;
        }

        Serial.println("[tailscale] Auth key saved. Rebooting...");
        Serial.flush();
        delay(300);
        ESP.restart();
        return true;
    }

    if (line == "vpn-clear") {
        Preferences prefs;

        if (prefs.begin("sixback-vpn", false)) {
            prefs.remove("authkey");
            prefs.end();
        }

        Serial.println("[tailscale] Auth key cleared. Rebooting...");
        Serial.flush();
        delay(300);
        ESP.restart();
        return true;
    }

    return false;
}

void startMicrolink() {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(
            "[tailscale] WiFi unavailable, skipping startup"
        );
        return;
    }

    // Read credentials from NVS, not from compiled firmware.
    Preferences prefs;

    if (!prefs.begin("sixback-vpn", true)) {
        Serial.println(
            "[tailscale] VPN credentials not provisioned"
        );
        return;
    }

    gAuthKey = prefs.getString("authkey", "");
    prefs.end();

    if (gAuthKey.isEmpty()) {
        Serial.println(
            "[tailscale] No auth key configured"
        );
        return;
    }

    microlink_config_t config = {};

    config.auth_key = gAuthKey.c_str();
    config.device_name = "sixback-bingley-combined";
    config.enable_derp = true;
    config.enable_stun = true;
    config.enable_disco = true;
    config.max_peers = 16;

    // Let SixBack retain control of WiFi transmit power.
    config.wifi_tx_power_dbm = 0;

    Serial.println("[tailscale] Initializing MicroLink");

    gMicrolink = microlink_init(&config);

    if (!gMicrolink) {
        Serial.println(
            "[tailscale] MicroLink initialization failed"
        );
        return;
    }

    microlink_set_state_callback(
        gMicrolink,
        onMicrolinkState,
        nullptr
    );

    esp_err_t result = microlink_start(gMicrolink);

    if (result != ESP_OK) {
        Serial.printf(
            "[tailscale] Startup failed: %s\n",
            esp_err_to_name(result)
        );
        return;
    }

    Serial.println(
        "[tailscale] MicroLink startup requested"
    );
}

} // namespace sixback

#else

namespace sixback {
void startMicrolink() {}
bool handleMicrolinkSerialCommand(const String&) { return false; }
}

#endif
