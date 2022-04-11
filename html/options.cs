public static class Configuration
{
    public const string HTML_DEFINE_PREAMBLE = "WIFI_CONFIG_HTML_";
    public static (string tag, string file)[] FilesToProcess = {
        ("WIFI_DETAILS", "html/wifi.html"),
        ("MQTT_DETAILS", "html/mqtt.html"),
        ("BOOT_DETAILS", "html/mode.html"),
        ("FORM_ERRPAGE", "html/ferr.html"),
        ("PAGE_CSSFILE", "html/style.css")
    };

    public const string OUTPUT_FILE_NAME = "htmlsrc.h";
    public const string OUTPUT_FILE_SUBDIR = "includes";
}