public static class Configuration
{
    public const string HTML_DEFINE_PREAMBLE = "WIFI_CONFIG_HTML_";
    public static (string tag, string file)[] FilesToProcess = {
        ("WIFI_DETAILS", "html/wifi.html"),
        ("WIFI_CONFIRM", "html/wfok.html"),
        ("MQTT_DETAILS", "html/mqtt.html"),
        ("MQTT_CONFIRM", "html/mqok.html"),
        ("DENY_BOOTNOW", "html/deny.html"),
        ("MODE_DETAILS", "html/mode.html"),
        ("DONE_CONFIRM", "html/done.html"),
        ("SAVE_CONFIRM", "html/save.html"),
        ("E400_ERRPAGE", "html/ferr.html"),
        ("E404_ERRPAGE", "html/none.html"),
        ("E500_ERRPAGE", "html/ierr.html"),
        ("ONLY_CSSFILE", "html/style.css")
    };

    public const string OUTPUT_FILE_NAME = "htmlsrc.h";
    public const string OUTPUT_FILE_SUBDIR = "includes";
}