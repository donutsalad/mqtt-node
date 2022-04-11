public class HTMLStripper
{
    private readonly string _html;
    private readonly string _processedHtml;

    public HTMLStripper(string filepath)
    {
        try
        {
            _html = File.ReadAllText(filepath);
        }
        catch (Exception e)
        {
            Console.WriteLine($"Exception occured whilst attempting to read html file \"{filepath}\": {e.Message}");
            _processedHtml = _html = "ERR_IN_HTML_PARSING_UTILITY";
            return;
        }

        _processedHtml = _html.Replace("\t", "").Replace("\n", "").Replace("\r", "").Replace("\"", "\\\"");
    }

    public string HTML => _processedHtml;
    public string CHeader(string defineTag) => $"#define {defineTag} \"{_processedHtml}\"";
}