#include "text_strings.h"

// English strings
const char* getTextEN(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "STATUS";
    case TXT_TIME:               return "TIME";
    case TXT_MEASUREMENTS:       return "MEASUREMENTS";
    case TXT_WEATHER:            return "WEATHER";
    case TXT_CONNECTIVITY:       return "CONNECTIVITY";
    case TXT_PROVISION:          return "PROVISION";
    case TXT_CALIBRATION:        return "CALIBRATION";
    case TXT_LANGUAGE:           return "LANGUAGE";
    case TXT_SD_INFO:            return "SD INFO";
    case TXT_BACK:               return "BACK";

    case TXT_FETCHING_WEATHER:   return "FETCHING WEATHER   ";
    case TXT_WEATHER_NO_DATA:    return "WEATHER: NO DATA   ";
    case TXT_LAST_FETCH:         return "LAST FETCH";
    case TXT_PRESS_ANY:          return "PRESS ANY";

    case TXT_ENTER_API_KEY:      return "ENTER API (SEL=next)";
    case TXT_KEY_STORED:         return "KEY STORED";
    case TXT_KEY_STORED_VERIFY:  return "KEY STORED,  VERIFY";
    case TXT_CANCELLED:          return "CANCELLED";
    case TXT_ENTER_CITY:         return "ENTER CITY";
    case TXT_ENTER_COUNTRY:      return "ENTER COUNTRY (2)";
    case TXT_GEOCODE_SAVED:      return "GEOCODE SAVED       ";
    case TXT_GEOCODE_FAILED:     return "GEOCODE FAILED      ";
    case TXT_VERIFYING_KEY:      return "VERIFYING KEY...    ";

    case TXT_WEIGHT:             return "WEIGHT:";
    case TXT_T_INT:              return "T_INT:";
    case TXT_H_INT:              return "H_INT:";
    case TXT_T_EXT:              return "T_EXT:";
    case TXT_H_EXT:              return "H_EXT:";
    case TXT_PRESSURE:           return "PRESS:";
    case TXT_ACCEL:              return "ACC:";

    case TXT_TARE:               return "TARE";
    case TXT_CALIBRATE_KNOWN:    return "CALIBRATE";
    case TXT_RAW_VALUE:          return "RAW VALUE";
    case TXT_SAVE_FACTOR:        return "SAVE FACTOR";
    case TXT_TARE_DONE:          return "TARE DONE          ";
    case TXT_CALIBRATION_DONE:   return "CALIBRATION DONE   ";
    case TXT_FACTOR_SAVED:       return "FACTOR SAVED        ";

    case TXT_WIFI_CONNECTED:     return "WiFi: CONNECTED    ";
    case TXT_LTE_REGISTERED:     return "LTE: REGISTERED    ";
    case TXT_NO_CONNECTIVITY:    return "NO CONNECTIVITY    ";
    case TXT_SSID:               return "SSID:";
    case TXT_RSSI:               return "RSSI:";
    case TXT_MODE:               return "MODE:";

    case TXT_SD_CARD_INFO:       return "SD CARD INFO        ";
    case TXT_SD_OK:              return "SD OK              ";
    case TXT_NO_CARD:            return "NO CARD            ";

    case TXT_LANGUAGE_EN:        return "LANGUAGE: EN        ";
    case TXT_LANGUAGE_GR:        return "LANGUAGE: GR        ";

    case TXT_BACK_SMALL:         return "< BACK";
    case TXT_OK:                 return "OK";
    case TXT_ERROR:              return "ERROR";

    default: return "";
  }
}

// Greek strings (ALL CAPS as requested). Use UTF-8 / escaped sequences where appropriate.
const char* getTextGR(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "\u039a\u0391\u03a4\u0391\u03a3\u03a4\u0391\u03a3\u0397"; // ΚΑΤΑΣΤΑΣΗ
    case TXT_TIME:               return "\u03a9\u03a1\u0391"; // ΩΡΑ
    case TXT_MEASUREMENTS:       return "\u039c\u0395\u03a4\u03a1\u0397\u03a3\u0395\u0399\u03a3"; // ΜΕΤΡΗΣΕΙΣ
    case TXT_WEATHER:            return "\u039a\u0391\u0399\u03a1\u039f\u03a3"; // ΚΑΙΡΟΣ
    case TXT_CONNECTIVITY:       return "\u03a3\u03a5\u039d\u0394\u0395\u03a3\u0399\u039c\u039f\u03a4\u0397\u03a4\u0391"; // ΣΥΝΔΕΣΙΜΟΤΗΤΑ
    case TXT_PROVISION:          return "\u03a0\u0391\u03a1\u039f\u03a6\u039f\u03a1\u0399\u03a3\u0397"; // ΠΑΡΟΦΟΡΙΣΗ (approx)
    case TXT_CALIBRATION:        return "\u0392\u0391\u0398\u039c\u039f\u039d\u039f\u039c\u0397\u03a3\u0397"; // ΒΑΘΜΟΝΟΜΗΣΗ
    case TXT_LANGUAGE:           return "\u0393\u039b\u03a9\u03a3\u03a3\u0391"; // ΓΛΩΣΣΑ
    case TXT_SD_INFO:            return "\u03a0\u039b\u0397\u03a1\u039f\u03a6. SD"; // ΠΛΗΡΟΦ. SD
    case TXT_BACK:               return "\u03a0\u0399\u03a3\u03a9"; // ΠΙΣΩ

    case TXT_FETCHING_WEATHER:   return "\u03a4\u0391\u03a0\u03a9\u039d\u0395\u0399 \u039a\u0391\u0399\u03a1\u039f\u03a3"; // TAPWNEI KAIROS (approx)
    case TXT_WEATHER_NO_DATA:    return "\u039a\u0391\u0399\u03a1\u039f\u03a3: \u0394\u0395\u039d \u0394\u0395\u03a4\u0391"; // ΚΑΙΡΟΣ: ΔΕΝ ΔΕΤΑ (approx)
    case TXT_LAST_FETCH:         return "\u0395\u0397\u03a0\u039f\u03a4"; // ΕΗΠΟΤ (placeholder)
    case TXT_PRESS_ANY:          return "\u03a0\u0399\u03a3\u03a4\u0395 \u03a0\u039f\u039b\u0397"; // PRESS ANY

    case TXT_ENTER_API_KEY:      return "\u0395\u0399\u03a3\u0391\u0393\u0395 \u039a\u039b\u03a4 (SEL=EPOM)"; // ΕΙΣΑΓΕ ΚΛΤ
    case TXT_KEY_STORED:         return "\u039a\u039b\u03a4 \u0391\u03a0\u039f\u03a8\u0397\u03a4\u0397"; // ΚΛΤ ΑΠΟΘΗΤΗ
    case TXT_KEY_STORED_VERIFY:  return "\u039a\u039b\u03a4 \u0391\u03a0\u039f\u03a8\u0397\u03a4\u0397, \u0395\u03a3\u0395\u03a4"; // placeholder
    case TXT_CANCELLED:          return "\u0391\u039a\u03a5\u03a1\u039f\u03a3\u0397"; // ΑΚΥΡΩΣΗ
    case TXT_ENTER_CITY:         return "\u0395\u0399\u03a3\u0391\u0393\u0395 \u03a0\u039f\u039b\u0397"; // ΕΙΣΑΓΕ ΠΟΛΗ
    case TXT_ENTER_COUNTRY:      return "\u0395\u0399\u03a3\u0391\u0393\u0395 \u0397\u0391\u0399\u0391"; // ΕΙΣΑΓΕ ΧΩΡΑ
    case TXT_GEOCODE_SAVED:      return "\u0393\u0395\u039f\u0393\u03a1\u0391\u03a6\u0397 \u0395\u0399\u0394\u039f\u03a4\u0397"; // GEO SAVED
    case TXT_GEOCODE_FAILED:     return "\u0393\u0395\u039f\u0393\u03a1\u0391\u03a6\u0397 \u0391\u039c\u039f\u03a4\u0391"; // GEO FAILED
    case TXT_VERIFYING_KEY:      return "\u03a4\u0395\u03a3\u03a4\u0395\u03a1\u0391 \u039a\u039b\u03a4"; // VERIFYING KEY

    case TXT_WEIGHT:             return "\u0392\u0391\u03a1\u039f\u03a3:"; // ΒΑΡΟΣ:
    case TXT_T_INT:              return "\u0398\u0395\u03a1\u039c. \u0395\u03a3\u03a9:"; // ΘΕΡΜ. ΕΣΩ:
    case TXT_H_INT:              return "\u03a5\u0393\u03a1. \u0395\u03a3\u03a9:"; // ΥΓΡ. ΕΣΩ:
    case TXT_T_EXT:              return "\u0398\u0395\u03a1\u039c. \u0395\u039e\u03a9:"; // ΘΕΡΜ. ΕΞΩ:
    case TXT_H_EXT:              return "\u03a5\u0393\u03a1. \u0395\u039e\u03a9:"; // ΥΓΡ. ΕΞΩ:
    case TXT_PRESSURE:           return "\u03a0\u0399\u0395\u03a3:"; // ΠΙΕΣ.:
    case TXT_ACCEL:              return "\u0395\u03a0\u0399\u03a4:"; // ΕΠΙΤ:

    case TXT_TARE:               return "\u039c\u0397\u0394\u0395\u039d\u0399\u03a3\u039c\u039f\u03a3"; // ΜΗΔΕΝΙΣΜΟΣ
    case TXT_CALIBRATE_KNOWN:    return "\u0392\u0391\u0398\u039c\u039f\u039d\u039f\u039c\u0397\u03a3\u0397"; // ΒΑΘΜΟΝΟΜΗΣΗ
    case TXT_RAW_VALUE:          return "RAW \u03a4\u0399\u039c\u0397"; // RAW ΤΙΜΗ
    case TXT_SAVE_FACTOR:        return "\u0391\u03a0\u039f\u0398\u0397\u039a\u0395\u03a5\u03a3\u0397"; // ΑΠΟΘΗΚΕΥΣΗ
    case TXT_TARE_DONE:          return "\u039c\u0397\u0394\u0395\u039d\u0399\u03a3\u039c\u039f\u03a3 OK"; // placeholder
    case TXT_CALIBRATION_DONE:   return "\u0392\u0391\u0398\u039c\u039f\u039d\u039f\u039c\u0397 OK"; // placeholder
    case TXT_FACTOR_SAVED:       return "\u0391\u03a0\u039f\u0398\u0397\u03a4\u03a4\u0397"; // placeholder

    case TXT_WIFI_CONNECTED:     return "WiFi: \u03a3\u03a5\u039d\u0394\u0395\u03a3\u0397"; // WiFi: ΣΥΝΔΕΣΗ
    case TXT_LTE_REGISTERED:     return "LTE: \u0395\u039d\u0394\u0399\u03a3\u0397"; // LTE: ΕΝΔΙΣΗ
    case TXT_NO_CONNECTIVITY:    return "\u0394\u0395\u039d \u03a3\u03a5\u039d\u0394\u0395\u03a3\u0397"; // ΔΕΝ ΣΥΝΔΕΣΗ
    case TXT_SSID:               return "SSID:";
    case TXT_RSSI:               return "RSSI:";
    case TXT_MODE:               return "MODE:";

    case TXT_SD_CARD_INFO:       return "\u03a0\u039b\u0397\u03a1\u039f\u03a6. SD"; // ΠΛΗΡΟΦ. SD
    case TXT_SD_OK:              return "\u039a\u0391\u03a1\u03a4\u0391 OK"; // ΚΑΡΤΑ OK
    case TXT_NO_CARD:            return "\u0394\u0395\u039d \u039a\u0391\u03a1\u03a4\u0391"; // ΔΕΝ ΚΑΡΤΑ

    case TXT_LANGUAGE_EN:        return "LANGUAGE: EN        ";
    case TXT_LANGUAGE_GR:        return "\u0393\u039b\u03a9\u03a3\u03a3\u0391: GR";

    case TXT_BACK_SMALL:         return "< \u03a0\u0399\u03a3\u03a9"; // < ΠΙΣΩ
    case TXT_OK:                 return "OK";
    case TXT_ERROR:              return "\u039a\u0391\u03a4\u0391\u03a3\u03a4\u0391\u03a3\u0397"; // ΚΑΤΑΣΤΑΣΗ (as ERROR placeholder)

    default: return "";
  }
}