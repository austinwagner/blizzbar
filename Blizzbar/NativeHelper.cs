using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Blizzbar.Agent;


namespace Blizzbar
{
    internal sealed class NativeHelper : IDisposable
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct UpdateHandlerResult
        {
            public IntPtr UpdateHandler;
            public IntPtr ErrorMessage;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct GameInfo
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string Exe32;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string Exe64;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
            public string AppUserModelId;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
            public string RelaunchCommand;
        }

        [DllImport("BlizzbarNativeHelper32.dll")]
        private static extern UpdateHandlerResult NewUpdateHandler();

        [DllImport("BlizzbarNativeHelper32.dll")]
        private static extern void FreeUpdateHandler(IntPtr updateHandler);

        [DllImport("BlizzbarNativeHelper32.dll")]
        private static extern void FreeErrorMessage(IntPtr errorMessage);

        [DllImport("BlizzbarNativeHelper32.dll")]
        private static extern IntPtr SetGameInfo(IntPtr updateHandler, GameInfo[] gameInfo, UIntPtr gameInfoLen);

        private IntPtr updateHandler;

        public NativeHelper()
        {
            var result = NewUpdateHandler();
            if (result.UpdateHandler == IntPtr.Zero)
            {
                var message = Marshal.PtrToStringUni(result.ErrorMessage);
                FreeErrorMessage(result.ErrorMessage);
                throw new Exception(message);
            }

            updateHandler = result.UpdateHandler;
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~NativeHelper()
        {
            this.Dispose(false);
        }

        private void Dispose(bool disposing)
        {
            if (updateHandler != IntPtr.Zero)
            {
                FreeUpdateHandler(updateHandler);
                this.updateHandler = IntPtr.Zero;
            }
        }

        public void SetGameDetails(List<GameDetails> details)
        {
            var info = details.Select(x =>
                new GameInfo
                {
                    Exe32 = x.Exe32,
                    Exe64 = x.Exe64,
                    AppUserModelId = "BlizzardEntertainment.BattleNet." + x.ShortName,
                    RelaunchCommand = "battlenet://" + x.ShortName,
                }).ToArray();

            SetGameInfo(this.updateHandler, info, (UIntPtr) (uint) info.Length);
        }
    }
}
