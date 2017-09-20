using System;

namespace Blizzbar.Interop
{
    [Flags]
    internal enum SendMessageTimeoutFlags : uint
    {
        Normal = 0x0,
        Block = 0x1,
        AbortIfHung = 0x2,
        NoTimeoutIfHung = 0x8,
        ErrorOnExit = 0x20
    }
}