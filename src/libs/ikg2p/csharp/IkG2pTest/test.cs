using IKg2p;
using System.Diagnostics;
using System.Reflection;
using System.Text;

namespace IKg2pTest
{
    class Program
    {
        static string[] ReadData(string sourceName)
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            string resourceName = assembly.GetName().Name + "." + sourceName;
            using (Stream stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream != null)
                {
                    using (StreamReader reader = new StreamReader(stream, Encoding.UTF8))
                    {
                        string content = reader.ReadToEnd();
                        return content.Split(new[] { Environment.NewLine }, StringSplitOptions.None);
                    }
                }
                else
                {
                    Console.WriteLine("Resource not found.");
                    return new string[0];
                }
            }
        }
        static void Main(string[] args)
        {
            string[] dataLines;

            // mandarin or cantonese
            bool mandarin = true;
            bool resDisplay = true;

            ZhG2p zhG2p;
            if (mandarin)
            {
                zhG2p = new ZhG2p("mandarin");
                dataLines = ReadData("op_lab.txt");
            }
            else
            {
                zhG2p = new ZhG2p("cantonses");
                dataLines = ReadData("jyutping_test.txt");
            }

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            StreamWriter writer = new StreamWriter("out.txt");
            int count = 0;
            int error = 0;
            if (dataLines.Length > 0)
            {
                foreach (string line in dataLines)
                {
                    string trimmedLine = line.Trim();
                    string[] keyValuePair = trimmedLine.Split('|');


                    if (keyValuePair.Length == 2)
                    {
                        string key = keyValuePair[0];
                        string value = keyValuePair[1];
                        string result = zhG2p.Convert(key, false, true);
                        // var result = TinyPinyin.PinyinHelper.GetPinyin(key).ToLower();

                        var words = value.Split(" ");
                        int wordSize = words.Length;
                        count += wordSize;

                        bool diff = false;
                        var resWords = result.Split(" ");
                        for (int i = 0; i < wordSize; i++)
                        {
                            if (words[i] != resWords[i] && !words[i].Split("/").Contains(resWords[i]))
                            {
                                diff = true;
                                error++;
                            }
                        }

                        if (resDisplay && diff)
                        {
                            Console.WriteLine("text: " + key);
                            Console.WriteLine("raw: " + value);
                            Console.Write("out:");
                            writer.WriteLine(trimmedLine);

                            for (int i = 0; i < wordSize; i++)
                            {
                                if (words[i] != resWords[i] && !words[i].Split("/").Contains(resWords[i]))
                                {
                                    Console.ForegroundColor = ConsoleColor.Red;
                                    Console.Write(" " + resWords[i]);
                                }
                                else
                                {
                                    Console.ResetColor();
                                    Console.Write(" " + resWords[i]);
                                }
                            }
                            Console.ResetColor();
                            Console.WriteLine();
                            Console.WriteLine();
                        }
                    }
                }
                writer.Close();
                stopwatch.Stop();

                double percentage = Math.Round(((double)error / (double)count) * 100.0, 2);
                Console.WriteLine("错误率: " + percentage + "%");
                Console.WriteLine("错误数: " + error);
                Console.WriteLine("总字数: " + count);

                TimeSpan elapsedTime = stopwatch.Elapsed;
                Console.WriteLine("函数执行时间: " + elapsedTime.TotalMilliseconds + " 毫秒");
            }
        }
    }
}