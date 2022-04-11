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

File.WriteAllLines(outputFile, Configuration.FilesToProcess.ToList().ConvertAll<string>(
    tuple => new HTMLStripper(tuple.file).CHeader(tuple.tag)
));

Console.WriteLine($"Done! Output File Contents:\n{File.ReadAllText(outputFile)}");