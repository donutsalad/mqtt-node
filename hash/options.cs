public static class Configuration
{
    public const string OUTPUT_FILE_NAME = "pchashes.h";
    public const string OUTPUT_FILE_SUBDIR = "includes";
    
    public static uint HASH_FUNCTION(string str)
    {
        uint hash = 5381;
        for (int i = 0; i < str.Length; i++)
            hash = 33 * hash ^ (byte) str[i];

        return hash;
    }

    //Currently doesn't work with spaces thanks to the headerfile! Need to sub _ for ' ' when printing to .h
    public static List<string> precomputeTargets = new List<string>
    {
        "cmd",
        "sys",
        "app",
        "poll",
        "control",

        "HelloWorld"
    };
}