string SanityCheck = "We're currently using the DBJ (v2) Hash Alorithm";
Console.WriteLine(SanityCheck);
Console.WriteLine(
    "And the above, hashed therein is: " + 
    Configuration.HASH_FUNCTION(SanityCheck) + " | " + 
    Convert.ToString(Configuration.HASH_FUNCTION(SanityCheck), 16)
);

Console.WriteLine("Parent Directory Input:\nPress [d] to use default directory. Otherwise press any other key and then enter a path to use a custom directory.");
string outputFile = Path.Combine(
    Console.ReadKey(true).KeyChar == 'd' ? "../main" : Console.ReadLine() ?? "../main", 
    Path.Combine(Configuration.OUTPUT_FILE_SUBDIR, Configuration.OUTPUT_FILE_NAME)
);

Console.WriteLine($"Output will be written to: {outputFile}\nIs this correct? (y/n)");
if (Console.ReadKey(true).KeyChar != 'y')
{
    Console.WriteLine("Aborting...");
    return;
}

string directory = Path.GetDirectoryName(outputFile) ?? "ERROR"; if(directory == "ERROR") 
{
    Console.WriteLine("ERROR: Could not get directory name of output file, Aborting...");
    return;
}

if(!Directory.Exists(directory))
{
    Console.WriteLine($"Directory \"{directory}\" does not exist, creating...");
    Directory.CreateDirectory(directory);
}

else if(File.Exists(outputFile))
{
    Console.WriteLine("Output file already exists, overwrite? (y/n)");
    if (Console.ReadKey(true).KeyChar != 'y')
    {
        Console.WriteLine("Aborting...");
        return;
    }
}

List<(string, uint)> hashes = Configuration.precomputeTargets.ConvertAll(target => (target, Configuration.HASH_FUNCTION(target)));
int cushioning = ((hashes.Max(((string input, uint hash) x) => x.input.Length) / 4) + 1) * 4; // At least one tab before value.

Console.WriteLine("Hashing output");
hashes.ForEach(((string input, uint hash) x) => Console.WriteLine(
    $"{x.input}:{new string(' ', cushioning - x.input.Length)}{x.hash:0000000000} | 0x{Convert.ToString(x.hash, 16)}"
));

if(hashes.ConvertAll<uint>(((string input, uint hash) x) => x.hash).Distinct().ToList().Count != hashes.Count)
{
    Console.WriteLine("ERROR: Collision detected! You might want to tweak the strings or hash function. Aborting...");
    return;
}

File.WriteAllLines(outputFile, hashes.ConvertAll(((string input, uint hash) x)  =>
    $"#define PCHASH_{x.input}{new string(' ', cushioning - x.input.Length)}{x.hash:0000000000}"
));

Console.WriteLine($"Done! Output File Contents:\n{File.ReadAllText(outputFile)}");