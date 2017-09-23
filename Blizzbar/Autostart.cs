using System;
using System.IO;
using System.Reflection;
using Microsoft.Win32;

namespace Blizzbar
{
    internal static class Autostart
    {
        private const string KeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string ValueName = @"Blizzbar";

        public static bool IsRegistered()
        {
            using (var key = Registry.CurrentUser.OpenSubKey(KeyPath, false))
            {
                var rawValue = key?.GetValue(ValueName) as string;
                if (rawValue == null) return false;

                var path1 = Path.GetFullPath(rawValue);
                var path2 = Path.GetFullPath(Assembly.GetEntryAssembly().Location);

                return string.Equals(path1, path2, StringComparison.CurrentCultureIgnoreCase);
            }
        }

        public static void Register()
        {
            using (var key = Registry.CurrentUser.CreateSubKey(KeyPath, true))
            {
                var path = Path.GetFullPath(Assembly.GetEntryAssembly().Location);
                key.SetValue(ValueName, path);
            }
        }

        public static void Unregister()
        {
            using (var key = Registry.CurrentUser.OpenSubKey(KeyPath, true))
            {
                key?.DeleteValue(ValueName, false);
            }
        }
    }
}