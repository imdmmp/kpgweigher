﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using System.Windows;
using System.ComponentModel;
using System.Threading;
namespace ioex_cs
{
    enum NodeStatus
    {
        ST_IDLE,
        ST_LOST,
        ST_BUSY,
    }

    enum NodeType : byte
    {
        BOARD_TYPE_VIBRATE = 0,
        BOARD_TYPE_WEIGHT = 1,
        BOARD_TYPE_COLLECT = 2,
        BOARD_TYPE_INVALID = 15
    }
    enum NodeGroup : byte
    {
        BOARD_GROUP_1 = 0,
        BOARD_GROUP_2,
        BOARD_GROUP_3,
        BOARD_GROUP_4
    }

    internal class VibrateNode : SubNode
    {
        public bool OnOff
        {
            get { return true; }
            set{}
        }
        public UInt16 Amp { get; set; }
        public UInt16 Freq { get; set; }
        internal VibrateNode(SPort _port,byte node_addr): base(_port,node_addr)
        {

        }
        public void FromElement(XElement cfgNode)
        {
            Amp = UInt16.Parse(cfgNode.Element("Amplitude").Value);
            Freq = UInt16.Parse(cfgNode.Element("Frequency").Value);
        }
        public void ToElement(XElement cfgNode)
        {
            cfgNode.Add(new XElement("Amplitude", Amp.ToString()));
            cfgNode.Add(new XElement("Frequency", Freq.ToString()));
        }
        public override void Action(string action, bool Wait)
        {
            (Application.Current as App).bMainPause = true;
            base.Action(action, Wait);
            (Application.Current as App).bMainPause = false;
        }
    }
    internal class Intf
    {
        public UInt16 buf { get; set; }

        public Intf(UInt16 ini_intf)
        {
            buf = ini_intf;
        }
        private UInt16 getbits(byte start, byte end) //get several bits based on bit position start from 0
        {
            UInt16 i = (UInt16)1;
            UInt16 j = (UInt16)1;
            while (start-- > end)
            {
                j = (UInt16)(j * 2);
            }
            while (end-- > 0)
            {
                i = (UInt16)(i * 2);
            }
            return (UInt16)((UInt16)(buf / i) - (UInt16)((UInt16)(buf / j) * j));
        }
        public bool b_Handshake
        {
            get
            {
                return (getbits(8, 7) == 1);
            }
            set
            {
                if (value)
                {
                    buf += 128;
                }
            }
        }
        public bool b_Hasmem
        {
            get
            {
                return (getbits(2, 1) == 1);
            }
            set
            {
                if (value)
                {
                    buf += 2;
                }
            }
        }
        public bool b_Hasdelay
        {
            get
            {
                return (getbits(3, 2) == 1);
            }
            set
            {
                if (value)
                {
                    buf += 4;
                }
            }
        }
        public UInt16 fmt_input
        {
            get { return getbits(7, 5); }
            set
            {
                if (value < 4)
                {
                    buf += (UInt16)(value * 32);
                }
            }
        }
        public UInt16 fmt_output
        {
            get { return getbits(5, 3); }
            set
            {
                if (value < 4)
                {
                    buf += (UInt16)(value * 8);
                }
            }
        }
        public UInt16 feed_times
        {
            get { return getbits(11, 8); }
            set
            {
                if (value < 8)
                {
                    buf += (UInt16)(value * 256);
                }
            }
        }
        public UInt16 delay_length
        {
            get { return getbits(16, 11); }
            set
            {
                if (value < 32)
                {
                    buf += (UInt16)(value * 2048);
                }
            }
        }
    }
        
    internal class BottomPackNode : SubNode
    {
        public Intf intf_byte;
        
        public UInt32 mode
        {
            get { return 0; }
            set {}
        }
        
        public void FromElement(XElement cfgNode)
        {
            intf_byte.b_Handshake = bool.Parse(cfgNode.Element("Handshake").Value);
            intf_byte.b_Hasdelay = bool.Parse(cfgNode.Element("Hasdelay").Value);
            intf_byte.b_Hasmem = bool.Parse(cfgNode.Element("Hasmem").Value);
            intf_byte.delay_length = UInt16.Parse(cfgNode.Element("delay_length").Value);
            intf_byte.feed_times = UInt16.Parse(cfgNode.Element("feed_times").Value);
            intf_byte.fmt_input = UInt16.Parse(cfgNode.Element("fmt_input").Value);
            intf_byte.fmt_output = UInt16.Parse(cfgNode.Element("fmt_output").Value);
        }
        public void ToElement(XElement cfgNode)
        {
            cfgNode.Add(new XElement("Handshake", intf_byte.b_Handshake.ToString()));
            cfgNode.Add(new XElement("Hasdelay", intf_byte.b_Hasdelay.ToString()));
            cfgNode.Add(new XElement("Hasmem", intf_byte.b_Hasmem.ToString()));
            cfgNode.Add(new XElement("delay_length", intf_byte.delay_length.ToString()));
            cfgNode.Add(new XElement("feed_times", intf_byte.feed_times.ToString()));
            cfgNode.Add(new XElement("fmt_input", intf_byte.fmt_input.ToString()));
            cfgNode.Add(new XElement("fmt_output", intf_byte.fmt_output.ToString()));
        }
        public  void SetInterface() //composition of interface parameter
        {
            //todo set interface
        }

        public void getInterface() //decomposition of interface parameter
        {
            //todo parse interface
        }
        public void TriggerPack()
        {
            //todo trigger pack
        }
        internal BottomPackNode(SPort _port, byte node_addr)
            : base(_port, node_addr)
        {
            intf_byte = new Intf(0);
        }
        public override void Action(string action, bool Wait)
        {
            (Application.Current as App).bMainPause = true;
            base.Action(action, Wait);
            
            (Application.Current as App).bMainPause = false;
        }
    }
    internal class WeighNode : SubNode
    {
        private double _weight;
        private double off;

        public bool bRelease { get; set; }
        public double weight
        {
            get {
                return _weight;
            }
            set{
                if (!this["Mtrl_Weight_gram"].HasValue)
                {
                    _weight = -1000000.0;
                }
                
                _weight = (double)(this["Mtrl_Weight_gram"].Value) +(double)this["Mtrl_Weight_decimal"].Value / (double)16.0; 

            }
        }
        public void NextCycle()
        {

        }
        
        public void DoCalibration(byte slot, UInt32 weight)
        {
            this["cs_mtrl"] = null;
            if (slot == 0) //zero point calibration
            {
                this["cs_zero"] = this["cs_cs_mtrl"];
                return;
            }else{
                this["Poise_Weight_gram" + slot.ToString()] = weight;
                this["cs_poise" + slot.ToString()] = this["cs_mtrl"];
            }
        }
        public override void Action(string action, bool Wait)
        {
            if (action == "loopquery") //query the weight and status
            {
                WaitForIdle();
                status = NodeStatus.ST_BUSY;
                read_regs(new string[] { "Mtrl_Weight_gram", "Mtrl_Weight_decimal", "status" });
                //read_regs(new string[] { "Mtrl_Weight_gram", "Mtrl_Weight_decimal" });
                WaitForIdle();
                return;
            }
            if (action == "release")
            {
                this["flag_enable"] = 4;
                this.bRelease = true;
            }

            (Application.Current as App).bMainPause = true;
            base.Action(action, Wait);
            
            if (action == "query") //query the weight and status
            {
                status = NodeStatus.ST_BUSY;
                read_regs(new string[] { "Mtrl_Weight_gram", "Mtrl_Weight_decimal", "status" });
                //read_regs(new string[] { "Mtrl_Weight_gram", "Mtrl_Weight_decimal" });
            }
            if (action == "zero")
            {
                this["flag_enable"] = 7;
            }
            if (action == "empty" || action == "stop")
            {
                this["flag_enable"] = 4;
            }
            if (action == "pass")
            {
                
                this["flag_enable"] = 3;

            }
            if (action == "fill")
            {
                this["flag_enable"] = 5;
            }
            WaitForIdle();
            (Application.Current as App).bMainPause = false;
            return;
        }
        internal WeighNode(SPort _port, byte node_addr)
            : base(_port,node_addr)
        {
            off = 0;
        }
    }
    /*
     * Manage XML file based config file, see app_config.xml for details
     * <Item Name="Id">
     *  <value>xxx</value>
     * </Item>
     */
    internal class XmlConfig
    {
        private Dictionary<string, string> curr_conf; //store the current configuration
        public string cfg_name;
        public XmlConfig(string xml_file)
        {
            curr_conf = new Dictionary<string, string>();
            this.xml_file = xml_file;
        }
        private string xml_file;
        
        public bool LoadConfigFromFile()
        {
            try
            {
                XDocument xml_doc;
                xml_doc = XDocument.Load(xml_file);
                cfg_name = xml_doc.Element("Root").Attribute("current").Value;
                IEnumerable<XElement> elem = xml_doc.Descendants("Item");
                foreach (XElement e in elem)
                {
                    curr_conf[e.Attribute("Name").Value] = e.ToString();
                }

                return true;
            }
            catch (System.Exception e)
            {
                MessageBox.Show(e.Message);
                return false;
            }
        }
        public bool SaveConfigToFile()
        {
            try
            {
                XElement all = new XElement("Root", new XAttribute("current", cfg_name));
                foreach (string cfg in curr_conf.Keys)
                {
                    XElement ecfg = XElement.Parse(curr_conf[cfg]);
                    ecfg.Add(new XAttribute("Name", cfg));
                    all.Add(ecfg);
                }
                XDocument newdoc = new XDocument(
                    new XDeclaration("1.0", "utf-8", "yes"),
                    all);
                newdoc.Save(xml_file);
                return true;
            }
            catch (System.Exception e)
            {
                MessageBox.Show(e.Message);
                return false;
            }
            
        }
        public void AddConfig(string cfgname,XElement e)
        {
            curr_conf[cfgname] = e.ToString();
        }
        public XElement LoadConfig(string newcfg)
        {
            if (curr_conf.ContainsKey(newcfg))
            {
                cfg_name = newcfg;
                return XElement.Parse(curr_conf[newcfg]);
            }
            return null;
        }
        public XElement RemoveConfig(string oldcfg)
        {
            if (curr_conf.Count > 1 && curr_conf.ContainsKey(oldcfg))
            {
                curr_conf.Remove(oldcfg);
                IEnumerable<String> keys = curr_conf.Keys;
                cfg_name = keys.GetEnumerator().Current;
                return LoadConfig(cfg_name);
            }
            return null;
        }
        public IEnumerable<String> Keys {
            get{
                return curr_conf.Keys;
            }
        }
    }
    
    internal struct RegType
    {
        public byte pos;    //start position of the register
        public byte size;   //size of the register
        public char rw;     //read/write property of the register, 'r','w','b'="rw","v"=volatile
        public char update; //indicate whether download/save at startup 
                     //'s': setting , 'i': initial setting, 'p': report 'a': action
    }
    internal class UIParam 
    {
        public static int last_nodeid;
        public bool selected { get; set; }
        static UIParam()
        {
            last_nodeid = -1;
        }
        public UIParam()
        {
            selected = false;
        }
        
    }
    internal abstract class SubNode : Object
    {

        private static Dictionary<string, RegType> reg_type_tbl; //table to map the reg name to details, should be loaded from xml file setting
        
        private static Dictionary<byte, string> reg_pos2name_tbl; //format, <2,baudrate> where 1 the multiplier, table to map the reg position to name, should be loaded from xml file
        private static Dictionary<byte, UInt32> reg_mulitply_tbl; //multiplier of each register
                
        private Dictionary<string, Nullable<UInt32>> curr_conf; //store the configuration string of each config name (<config_name, xml_value> pair>
        private XmlConfig all_conf;//store all the configurations

        public UIParam uidata;
        static string reg_define_file = "node_define.xml";
        private int timeout_cnt ; //count of timeout
        static int timeout_limit = 2; //command retry times
        public byte node_id { get; set; }
        public NodeStatus status { get; set; }
        public string errmsg
        {
            get
            {
                string ret = _errmsg;
                _errmsg = "";
                return ret;
            }
            set { _errmsg = value; }
        }
        private string _errmsg;
        private SPort port; //serial port
        
        static SubNode()
        {
            reg_mulitply_tbl = new Dictionary<byte, uint>();
            reg_type_tbl = new Dictionary<string, RegType>();
            reg_pos2name_tbl = new Dictionary<byte, string>();
            
            
            
            //load setting from xml file
            XDocument xml_doc;
            xml_doc = XDocument.Load(reg_define_file);
            IEnumerable<XElement> regs = xml_doc.Descendants("reg");
            foreach (XElement reg in regs)
            {
                byte size = byte.Parse(reg.Element("size").Value);
                byte offset = byte.Parse(reg.Element("offset").Value);
                char rw = char.Parse(reg.Element("rw").Value);
                char update = char.Parse(reg.Element("update").Value);
                string name = reg.Element("name").Value;
                RegType rt;
                rt.pos = offset;
                rt.size = size;
                rt.rw = rw;
                rt.update = update;
                reg_type_tbl[name] = rt;
                
                if ( size == 8)
                {
                    
                    reg_pos2name_tbl[offset] = name;
                    reg_mulitply_tbl[offset] = 1;
                }
                if (size == 16)
                {
                    
                    reg_pos2name_tbl[offset] = name;
                    reg_mulitply_tbl[offset] = 256;
                    reg_pos2name_tbl[(byte)(offset+1)] = name;
                    reg_mulitply_tbl[(byte)(offset+1)] = 1;
                }
                if (size == 32)
                {
                    reg_pos2name_tbl[offset] = name;
                    reg_mulitply_tbl[offset] = 256*256*256;
                    reg_pos2name_tbl[(byte)(offset + 1)] = name;
                    reg_mulitply_tbl[(byte)(offset + 1)] = 256 * 256;
                    reg_pos2name_tbl[(byte)(offset + 2)] = name;
                    reg_mulitply_tbl[(byte)(offset + 2)] = 256;
                    reg_pos2name_tbl[(byte)(offset + 3)] = name;
                    reg_mulitply_tbl[(byte)(offset + 3)] = 1;
                }
            }
            
            return;
        }
        internal SubNode(SPort _port, byte node_id)
        {
            port = _port;
            this.node_id = node_id;
            
            
            //to initial reg_table items from xml table
            curr_conf = new Dictionary<string, Nullable<UInt32>>();
            uidata = new UIParam();
            all_conf = new XmlConfig("node_" + node_id + ".xml");
            ICollection<string> names = reg_type_tbl.Keys;
            foreach (string name in names)
            {
                curr_conf[name] = null;
            }
            port.InFrameHandlers[node_id] = HandleInFrame;
            port.TimeoutHandlers[node_id]= HandleTimeout;
            
            timeout_cnt = 0;
            status = NodeStatus.ST_LOST;
            _errmsg = "";
            
        }
        private void write_regs(string[] names,UInt32[] values)
        {
            FrameBuffer ofrm = new FrameBuffer();
            ofrm.cmd = (byte)'W';
            if (! curr_conf["addr"].HasValue)
            {
                throw new Exception("Invalid node address");
            }
            ofrm.addr = (byte)curr_conf["addr"].Value;
            byte[] regbuffer = new byte[16];
            byte[] valbuffer = new byte[16];
            byte[] outbuffer = new byte[32];
            int outptr = 0;
            for (int i=0;i<names.GetLength(0);i++)
            {
                if (reg_type_tbl[names[i]].size == 8)
                {
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    valbuffer[outptr] = (byte)values[i];
                    outptr++;
                }
                if (reg_type_tbl[names[i]].size == 16)
                {
                    valbuffer[outptr] = (byte)(values[i]/256);
                    valbuffer[outptr+1] = (byte)(values[i]%256);
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    regbuffer[outptr+1] = (byte)(regbuffer[outptr] + 1);
                    outptr = outptr+2;
                }
                if (reg_type_tbl[names[i]].size == 32)
                {
                    valbuffer[outptr+3] = (byte)(values[i]%256);
                    values[i] = (UInt32)(values[i]/256);
                    valbuffer[outptr+2] = (byte)(values[i]%256);
                    values[i] = (UInt32)(values[i]/256);
                    valbuffer[outptr+1] = (byte)(values[i]%256);
                    values[i] = (UInt32)(values[i]/256);
                    valbuffer[outptr+0] = (byte)(values[i]%256);
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    regbuffer[outptr+1] = (byte)(regbuffer[outptr] + 1);
                    regbuffer[outptr+2] = (byte)(regbuffer[outptr] + 2);
                    regbuffer[outptr+3] = (byte)(regbuffer[outptr] + 3);

                    outptr = outptr+4;
                }
            }
            ofrm.generate_write_frame(outbuffer,regbuffer,valbuffer,0,(byte)(outptr));
            port.AddCommand(outbuffer);
        }
        public Nullable<UInt32> this[byte reg_id]    //get/set the reg_value
        {
            get { 
                return this[reg_pos2name_tbl[reg_id]]; 
            }
            set {
                this[reg_pos2name_tbl[reg_id]] = value; 
            }
        }
        public Nullable<UInt32> this[string reg_name]   //get/set the reg_value
        {
            get {
                return curr_conf[reg_name]; 
            }
            set {
                                
                if(status == NodeStatus.ST_BUSY)
                {
                    throw new Exception("reenter when busy");
                }
                status = NodeStatus.ST_BUSY;
                while (port.Status == PortStatus.BUSY)
                {
                    Thread.Sleep(1);
                }
                if (value.HasValue)
                {
                    if (curr_conf[reg_name].HasValue && curr_conf[reg_name].Value == value.Value)
                    {
                        if (reg_type_tbl[reg_name].rw != 'v') //non volatile value
                        {
                            return;
                        }
                    }
                    if (reg_type_tbl[reg_name].rw != 'r')
                    {
                        write_regs(new string[] { reg_name }, new UInt32[] { value.Value });
                        if (reg_type_tbl[reg_name].rw == 'v') //volatile value
                        {
                            status = NodeStatus.ST_IDLE;
                            return;
                        }
                    }
                    
                }
                if (reg_type_tbl[reg_name].rw != 'w')
                {
                    curr_conf[reg_name] = null;
                    read_regs(new string[] { reg_name });

                }
                while (status == NodeStatus.ST_BUSY)
                {
                    Thread.Sleep(1);
                }
                
                status = NodeStatus.ST_IDLE;
                    
            }
        }
        protected void read_regs(string[] names)
        {
            FrameBuffer ofrm = new FrameBuffer();
            ofrm.cmd = (byte)'R';
            ofrm.addr = node_id;
            byte[] regbuffer = new byte[16];
            byte[] outbuffer = new byte[16];
            int outptr = 0;
            for (int i = 0; i < names.GetLength(0); i++)
            {
                if (reg_type_tbl[names[i]].size == 8)
                {
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    outptr++;
                }
                if (reg_type_tbl[names[i]].size == 16)
                {
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    regbuffer[outptr + 1] = (byte)(regbuffer[outptr] + 1);
                    outptr = outptr + 2;
                }
                if (reg_type_tbl[names[i]].size == 32)
                {
                    regbuffer[outptr] = reg_type_tbl[names[i]].pos;
                    regbuffer[outptr + 1] = (byte)(regbuffer[outptr] + 1);
                    regbuffer[outptr + 2] = (byte)(regbuffer[outptr] + 2);
                    regbuffer[outptr + 3] = (byte)(regbuffer[outptr] + 3);

                    outptr = outptr + 4;
                }
            }
            ofrm.generate_read_frame(outbuffer, regbuffer, 0, (byte)(outptr));
            port.AddCommand(outbuffer);
        }
 
        //convert the current configuration into an XElement node
        private void SaveCurrentConfig()
        {
            XElement cfgNode = new XElement("Item");
            cfgNode.SetAttributeValue("Name", all_conf.cfg_name);
            foreach (string reg in curr_conf.Keys)
            {
                cfgNode.Add(new XElement(reg,curr_conf[reg]));
            }
            all_conf.AddConfig(all_conf.cfg_name, cfgNode);
            all_conf.SaveConfigToFile();
        }
        
        public void LoadCurrentConfig(string cfgname)
        {
            XElement cfgNode = all_conf.LoadConfig(cfgname);
            ////todo: load the default setting and download them
            return;
            foreach (string reg in curr_conf.Keys)
            {
                curr_conf[reg] = UInt32.Parse(cfgNode.Element(reg).Value);
            }
        }
        public void DuplicateCurrentConfig(string newcfg)
        {
            XElement cfgNode = new XElement("Item");
            cfgNode.SetAttributeValue("Name", newcfg);
            foreach (string reg in curr_conf.Keys)
            {
                cfgNode.Add(new XElement(reg, curr_conf[reg]));
            }
            all_conf.AddConfig(newcfg, cfgNode);
            all_conf.SaveConfigToFile();
            all_conf.LoadConfig(newcfg);
        }
        private void HandleTimeout(byte[] last_cmd)
        {
            if (timeout_cnt < timeout_limit)
            {
                timeout_cnt++;
                port.AddCommand(last_cmd);
                return;
            }
            errmsg = "timeout";
            status = NodeStatus.ST_LOST;
        }

        private void HandleInFrame(FrameBuffer ifrm)
        {
            if (ifrm.cmd == 'W') //write by bytes
            {
                for (int j = 0; j < ifrm.datalen; j=j+2 )
                {
                    try
                    {
                        string name = reg_pos2name_tbl[ifrm.data[j]];
                        UInt32 value = 0;
                        if (curr_conf[name].HasValue)
                        {
                            value = curr_conf[name].Value;
                        }
                        UInt32 multiplier = reg_mulitply_tbl[ifrm.data[j]];
                        curr_conf[name] = 256 * multiplier * (UInt32)(value / (256 * multiplier)) + multiplier * ifrm.data[j + 1] + (value % multiplier);
                    }
                    catch (System.Exception e)
                    {
                       MessageBox.Show(e.Message);                 	
                    }
                }
            }
            status = NodeStatus.ST_IDLE;
#region obsolete_readword_code
            /*
            if (ifrm.cmd == 'X')
            {
                for (int j = 0; j < ifrm.framelen;j=j+3 )
                {
                    try
                    {
                        //queryEvent(ifrm.data[j], (UInt32)(ifrm.data[j + 1]) * 256 + (UInt32)(ifrm.data[j + 2]));
                    }
                    catch (System.Exception e)
                    {
                        MessageBox.Show(e.Message);                 	
                    }
                }
            }
            */

#endregion
        }
        protected void WaitForIdle()
        {
            while (status == NodeStatus.ST_BUSY)
            {
                Thread.Sleep(1);
            }

        }

        protected void WaitUntilGetValue(string regname, UInt32 value)
        {
            Thread.Sleep(100);
            
            while(true)
            {
                if ((this[regname].HasValue && this[regname].Value == value))
                {
                    return;
                }
                this[regname] = null;
                status = NodeStatus.ST_IDLE;
                if (errmsg != "")
                {
                    status = NodeStatus.ST_IDLE;
                }
                Thread.Sleep(100);
            }
        }
        public void init_load_regs()  //load the initial reg value
        {
            foreach (string regname in reg_type_tbl.Keys)
            {
                RegType reg = reg_type_tbl[regname];
                if (reg.update == 's')
                {
                    this[regname] = null;
                }
            }
        }
        public virtual void Action(string action, bool Wait)  // empty, pass, fill, flash, zero, query
        {
            if (status == NodeStatus.ST_LOST)
                return;
            WaitForIdle();
            if (action == "stop")
            {
                status = NodeStatus.ST_IDLE;
                this["flag_enable"] = 0;
                Thread.Sleep(100);
            }  

            if (action == "start")
            {
                this["flag_enable"] = 1;
                Thread.Sleep(100);
            }  
            if (action == "flash")
            {
                //wait until all data is programed.
                this["NumOfDataToBePgmed"] = 45;
                if (errmsg == "")
                    WaitUntilGetValue("NumOfDataToBePgmed", 0);
            }
            WaitForIdle();
        }
    }
}
