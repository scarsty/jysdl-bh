

-- 配置文件
--为了简化处理，配置文件也用lua编写
--保存C程序读取的参数和lua程序中需要经常调整的参数。lua的其他参数仍然放在jyconst.lua中

CONFIG={};

CONFIG.Debug=1;         --输出调试和错误信息，=0不输出 =1 输出信息在debug.txt和error.txt到当前目录

--窗口设置类别。小于640*480(最小为320*240) 设为0，大于等于640*480 设为1
--目前只做了这两个类别，其他分辨率虽然可用，信息都能够显示，但是显示效果不一定好看。
--如果想在其他分辨率下美化显示效果，可以自行在jyconst.lua中修改相应的数据
CONFIG.Type= 1;

CONFIG.Width  = 800;       -- 游戏图形窗口宽, 设置为0则由系统自动适应
CONFIG.Height = 600;      -- 游戏图形窗口高

CONFIG.bpp  =32         -- 全屏时像素色深，一般为16或者32。在窗口模式时直接采用当前屏幕色深，此设置无效
                         -- 不支持8位色深。为提高速度，建议使用16位色深。
						 -- 24位未经过测试，不保证正确显示

CONFIG.FullScreen=0      -- 启动时是否全屏  1 全屏 0 窗口
CONFIG.EnableSound=1     -- 是否打开声音    1 打开 0 关闭   关闭了在游戏中无法打开


CONFIG.KeyRepeat=0       -- 是否激活键盘重复 0 不激活，只在走路菜单时键盘重复，1激活，包括对话的所有时候键盘均重复
CONFIG.KeyRepeatDelay =10;   --第一次键盘重复等待ms数
CONFIG.KeyRePeatInterval=100;  --一秒钟重复次数

CONFIG.XScale = 18    -- 贴图宽度的一半
CONFIG.YScale = 9     -- 贴图高度的一半

CONFIG.CharSet = 1			--游戏字体   0简体，1繁体

CONFIG.LargeMemory=0;             --设置内存使用方式 1 多使用内存，0 少使用内存


--设置各个数据文件的路径，如果是其他目录标志和windows不同的OS, 如linux，则改为合适的路径
CONFIG.Operation = 0;		--0:Windows	 1:android
if CONFIG.Operation == 1 then
	CONFIG.CurrentPath = "/sdcard/JYLDCR/";
	CONFIG.MP3 = 0;		--是否打开MP3
else
	CONFIG.CurrentPath = "./";
	CONFIG.LargeMemory = 1;
	CONFIG.MP3 = 1;		--是否打开MP3
end



CONFIG.DataPath=CONFIG.CurrentPath.."data/";
CONFIG.PicturePath=CONFIG.CurrentPath.."pic/";
CONFIG.SoundPath=CONFIG.CurrentPath.."sound/";
CONFIG.ScriptPath=CONFIG.CurrentPath.."script/";
CONFIG.CEventPath = CONFIG.ScriptPath .. "CEvent/"
CONFIG.WuGongPath = CONFIG.ScriptPath .. "WuGong/"
CONFIG.HelpPath = CONFIG.ScriptPath .. "Help/"

CONFIG.ScriptLuaPath = string.format("?.lua;%sscript/?.lua;%sscript/?.lua", CONFIG.CurrentPath, CONFIG.CurrentPath)

--CONFIG.ScriptLuaPath="?.lua;/sdcard/JYLDCR/script/?.lua;/sdcard/JYLDCR/script/?.lua";        --用lua规则写的路径

CONFIG.JYMain_Lua=CONFIG.ScriptPath .. "LDCR.lua";   --lua主程序名

CONFIG.FontName=CONFIG.CurrentPath.."font/font.ttc";

--使用FMOD播放MIDI，需要gm.dls文件
if CONFIG.MP3 == 0 then
	CONFIG.MidSF2 = CONFIG.SoundPath.."mid.sf2";
else
	CONFIG.MidSF2 = "";
end


--显示主地图x和y方向增加的贴图数，以保证所有贴图能全部显示
CONFIG.MMapAddX=2;
CONFIG.MMapAddY=2;
CONFIG.SMapAddX=2;
CONFIG.SMapAddY=16;
CONFIG.WMapAddX=2;
CONFIG.WMapAddY=16;

CONFIG.MusicVolume=50;            --设置播放音乐的音量(0-128)
CONFIG.SoundVolume=50;            --设置播放音效的音量(0-128)



if CONFIG.LargeMemory==1 then
     --贴图缓存数量，一般500-1000。如果在debug.txt中经常出现"pic cache is full"，可以适当增加
    CONFIG.MAXCacheNum=1000;
	CONFIG.CleanMemory=0;         --场景切换时是否清理lua内存。0 不清理 1 清理
	CONFIG.LoadFullS=1;           --1 整个S*文件载入内存 0 只载入当前场景，由于S*有4M多，这样可以解决很多内存
else
    CONFIG.MAXCacheNum=500;
	CONFIG.CleanMemory=1;
	CONFIG.LoadFullS=0;
end

--按键的位置，-1为默认位置
CONFIG.D1X = -1;
CONFIG.D1Y = -1;
CONFIG.D2X = -1;
CONFIG.D2Y = -1;
CONFIG.D3X = -1;
CONFIG.D3Y = -1;
CONFIG.D4X = -1;
CONFIG.D4Y = -1;
CONFIG.C1X = -1;
CONFIG.C1Y = -1;
CONFIG.C2X = -1;
CONFIG.C2Y = -1;
CONFIG.AX = -1;
CONFIG.AY = -1;
CONFIG.BX = -1;
CONFIG.BY = -1;

--贴图大小，百分比，100表示正常大小
CONFIG.Zoom = 100;

if CONFIG.Zoom > 100 then
	CONFIG.XScale = math.modf(CONFIG.XScale*CONFIG.Zoom/100)    -- 贴图宽度的一半
	CONFIG.YScale = math.modf(CONFIG.YScale*CONFIG.Zoom/100)     -- 贴图高度的一半
end

CONFIG.PlayName = "徐小侠"

CONFIG.Zoom=0;		--1近景  0远景
CONFIG.ShowWarInfo = 0;	--1战斗时显示主角信息条，0不显示
CONFIG.DrawMini = 0; --1默认打开迷你小地图，0不打开
CONFIG.ShowWarInstruction = 0	--1显示战斗说明，0不显示
CONFIG.KeyRepeat=0

CONFIG.Rotate=0
CONFIG.FastShowScreen = 0

