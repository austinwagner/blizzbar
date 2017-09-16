using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Blizzbar.Interop
{
    internal enum HookType : int
    {
        JournalRecord = 0,
        JournalPlayback = 1,
        Keyboard = 2,
        GetMessage = 3,
        CallWndProc = 4,
        Cbt = 5,
        SysMsgFilter = 6,
        Mouse = 7,
        Hardware = 8,
        Debug = 9,
        Shell = 10,
        ForegroundIdle = 11,
        CallWndProcRet = 12,
        KeyboardLow = 13,
        MouseLow = 14
    }
}
