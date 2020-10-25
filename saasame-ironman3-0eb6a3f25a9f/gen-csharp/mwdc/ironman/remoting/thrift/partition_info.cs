/**
 * Autogenerated by Thrift Compiler (0.9.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Thrift;
using Thrift.Collections;
#if !SILVERLIGHT
using System.Xml.Serialization;
#endif
using System.Runtime.Serialization;
using Thrift.Protocol;
using Thrift.Transport;

namespace mwdc.ironman.remoting.thrift
{

  #if !SILVERLIGHT
  [Serializable]
  #endif
  [DataContract(Namespace="")]
  public partial class partition_info : TBase
  {
    private int _disk_number;
    private string _drive_letter;
    private string _gpt_type;
    private string _guid;
    private bool _is_active;
    private bool _is_boot;
    private bool _is_hidden;
    private bool _is_offline;
    private bool _is_readonly;
    private bool _is_shadowcopy;
    private bool _is_system;
    private short _mbr_type;
    private long _offset;
    private int _partition_number;
    private long _size;

    [DataMember(Order = 0)]
    public THashSet<string> Access_paths { get; set; }

    [DataMember(Order = 0)]
    public int? Disk_number
    {
      get
      {
        return _disk_number;
      }
      set
      {
        __isset.disk_number = value.HasValue;
        if (value.HasValue) this._disk_number = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public string Drive_letter
    {
      get
      {
        return _drive_letter;
      }
      set
      {
        __isset.drive_letter = true;
        this._drive_letter = value;
      }
    }

    [DataMember(Order = 0)]
    public string Gpt_type
    {
      get
      {
        return _gpt_type;
      }
      set
      {
        __isset.gpt_type = true;
        this._gpt_type = value;
      }
    }

    [DataMember(Order = 0)]
    public string Guid
    {
      get
      {
        return _guid;
      }
      set
      {
        __isset.guid = true;
        this._guid = value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_active
    {
      get
      {
        return _is_active;
      }
      set
      {
        __isset.is_active = value.HasValue;
        if (value.HasValue) this._is_active = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_boot
    {
      get
      {
        return _is_boot;
      }
      set
      {
        __isset.is_boot = value.HasValue;
        if (value.HasValue) this._is_boot = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_hidden
    {
      get
      {
        return _is_hidden;
      }
      set
      {
        __isset.is_hidden = value.HasValue;
        if (value.HasValue) this._is_hidden = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_offline
    {
      get
      {
        return _is_offline;
      }
      set
      {
        __isset.is_offline = value.HasValue;
        if (value.HasValue) this._is_offline = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_readonly
    {
      get
      {
        return _is_readonly;
      }
      set
      {
        __isset.is_readonly = value.HasValue;
        if (value.HasValue) this._is_readonly = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_shadowcopy
    {
      get
      {
        return _is_shadowcopy;
      }
      set
      {
        __isset.is_shadowcopy = value.HasValue;
        if (value.HasValue) this._is_shadowcopy = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public bool? Is_system
    {
      get
      {
        return _is_system;
      }
      set
      {
        __isset.is_system = value.HasValue;
        if (value.HasValue) this._is_system = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public short? Mbr_type
    {
      get
      {
        return _mbr_type;
      }
      set
      {
        __isset.mbr_type = value.HasValue;
        if (value.HasValue) this._mbr_type = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public long? Offset
    {
      get
      {
        return _offset;
      }
      set
      {
        __isset.offset = value.HasValue;
        if (value.HasValue) this._offset = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public int? Partition_number
    {
      get
      {
        return _partition_number;
      }
      set
      {
        __isset.partition_number = value.HasValue;
        if (value.HasValue) this._partition_number = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public long? Size
    {
      get
      {
        return _size;
      }
      set
      {
        __isset.size = value.HasValue;
        if (value.HasValue) this._size = value.Value;
      }
    }


    [XmlIgnore] // XmlSerializer
    [DataMember(Order = 1)]  // XmlObjectSerializer, DataContractJsonSerializer, etc.
    public Isset __isset;
    #if !SILVERLIGHT
    [Serializable]
    #endif
    [DataContract]
    public struct Isset {
      [DataMember]
      public bool disk_number;
      [DataMember]
      public bool drive_letter;
      [DataMember]
      public bool gpt_type;
      [DataMember]
      public bool guid;
      [DataMember]
      public bool is_active;
      [DataMember]
      public bool is_boot;
      [DataMember]
      public bool is_hidden;
      [DataMember]
      public bool is_offline;
      [DataMember]
      public bool is_readonly;
      [DataMember]
      public bool is_shadowcopy;
      [DataMember]
      public bool is_system;
      [DataMember]
      public bool mbr_type;
      [DataMember]
      public bool offset;
      [DataMember]
      public bool partition_number;
      [DataMember]
      public bool size;
    }

    #region XmlSerializer support

    public bool ShouldSerializeDisk_number()
    {
      return __isset.disk_number;
    }

    public bool ShouldSerializeDrive_letter()
    {
      return __isset.drive_letter;
    }

    public bool ShouldSerializeGpt_type()
    {
      return __isset.gpt_type;
    }

    public bool ShouldSerializeGuid()
    {
      return __isset.guid;
    }

    public bool ShouldSerializeIs_active()
    {
      return __isset.is_active;
    }

    public bool ShouldSerializeIs_boot()
    {
      return __isset.is_boot;
    }

    public bool ShouldSerializeIs_hidden()
    {
      return __isset.is_hidden;
    }

    public bool ShouldSerializeIs_offline()
    {
      return __isset.is_offline;
    }

    public bool ShouldSerializeIs_readonly()
    {
      return __isset.is_readonly;
    }

    public bool ShouldSerializeIs_shadowcopy()
    {
      return __isset.is_shadowcopy;
    }

    public bool ShouldSerializeIs_system()
    {
      return __isset.is_system;
    }

    public bool ShouldSerializeMbr_type()
    {
      return __isset.mbr_type;
    }

    public bool ShouldSerializeOffset()
    {
      return __isset.offset;
    }

    public bool ShouldSerializePartition_number()
    {
      return __isset.partition_number;
    }

    public bool ShouldSerializeSize()
    {
      return __isset.size;
    }

    #endregion XmlSerializer support

    public partition_info() {
      this._disk_number = -1;
      this.__isset.disk_number = true;
      this._drive_letter = "";
      this.__isset.drive_letter = true;
      this._gpt_type = "";
      this.__isset.gpt_type = true;
      this._guid = "";
      this.__isset.guid = true;
      this._is_active = false;
      this.__isset.is_active = true;
      this._is_boot = false;
      this.__isset.is_boot = true;
      this._is_hidden = false;
      this.__isset.is_hidden = true;
      this._is_offline = false;
      this.__isset.is_offline = true;
      this._is_readonly = false;
      this.__isset.is_readonly = true;
      this._is_shadowcopy = false;
      this.__isset.is_shadowcopy = true;
      this._is_system = false;
      this.__isset.is_system = true;
      this._mbr_type = 0;
      this.__isset.mbr_type = true;
      this._offset = 0;
      this.__isset.offset = true;
      this._partition_number = -1;
      this.__isset.partition_number = true;
      this._size = 0;
      this.__isset.size = true;
    }

    public void Read (TProtocol iprot)
    {
      TField field;
      iprot.ReadStructBegin();
      while (true)
      {
        field = iprot.ReadFieldBegin();
        if (field.Type == TType.Stop) { 
          break;
        }
        switch (field.ID)
        {
          case 1:
            if (field.Type == TType.Set) {
              {
                Access_paths = new THashSet<string>();
                TSet _set0 = iprot.ReadSetBegin();
                for( int _i1 = 0; _i1 < _set0.Count; ++_i1)
                {
                  string _elem2;
                  _elem2 = iprot.ReadString();
                  Access_paths.Add(_elem2);
                }
                iprot.ReadSetEnd();
              }
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 2:
            if (field.Type == TType.I32) {
              Disk_number = iprot.ReadI32();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 3:
            if (field.Type == TType.String) {
              Drive_letter = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 4:
            if (field.Type == TType.String) {
              Gpt_type = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 5:
            if (field.Type == TType.String) {
              Guid = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 6:
            if (field.Type == TType.Bool) {
              Is_active = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 7:
            if (field.Type == TType.Bool) {
              Is_boot = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 8:
            if (field.Type == TType.Bool) {
              Is_hidden = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 9:
            if (field.Type == TType.Bool) {
              Is_offline = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 10:
            if (field.Type == TType.Bool) {
              Is_readonly = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 11:
            if (field.Type == TType.Bool) {
              Is_shadowcopy = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 12:
            if (field.Type == TType.Bool) {
              Is_system = iprot.ReadBool();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 13:
            if (field.Type == TType.I16) {
              Mbr_type = iprot.ReadI16();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 14:
            if (field.Type == TType.I64) {
              Offset = iprot.ReadI64();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 15:
            if (field.Type == TType.I32) {
              Partition_number = iprot.ReadI32();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 16:
            if (field.Type == TType.I64) {
              Size = iprot.ReadI64();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          default: 
            TProtocolUtil.Skip(iprot, field.Type);
            break;
        }
        iprot.ReadFieldEnd();
      }
      iprot.ReadStructEnd();
    }

    public void Write(TProtocol oprot) {
      TStruct struc = new TStruct("partition_info");
      oprot.WriteStructBegin(struc);
      TField field = new TField();
      if (Access_paths != null) {
        field.Name = "access_paths";
        field.Type = TType.Set;
        field.ID = 1;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteSetBegin(new TSet(TType.String, Access_paths.Count));
          foreach (string _iter3 in Access_paths)
          {
            oprot.WriteString(_iter3);
          }
          oprot.WriteSetEnd();
        }
        oprot.WriteFieldEnd();
      }
      if (__isset.disk_number) {
        field.Name = "disk_number";
        field.Type = TType.I32;
        field.ID = 2;
        oprot.WriteFieldBegin(field);
        oprot.WriteI32(Disk_number.Value);
        oprot.WriteFieldEnd();
      }
      if (Drive_letter != null && __isset.drive_letter) {
        field.Name = "drive_letter";
        field.Type = TType.String;
        field.ID = 3;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Drive_letter);
        oprot.WriteFieldEnd();
      }
      if (Gpt_type != null && __isset.gpt_type) {
        field.Name = "gpt_type";
        field.Type = TType.String;
        field.ID = 4;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Gpt_type);
        oprot.WriteFieldEnd();
      }
      if (Guid != null && __isset.guid) {
        field.Name = "guid";
        field.Type = TType.String;
        field.ID = 5;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Guid);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_active) {
        field.Name = "is_active";
        field.Type = TType.Bool;
        field.ID = 6;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_active.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_boot) {
        field.Name = "is_boot";
        field.Type = TType.Bool;
        field.ID = 7;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_boot.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_hidden) {
        field.Name = "is_hidden";
        field.Type = TType.Bool;
        field.ID = 8;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_hidden.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_offline) {
        field.Name = "is_offline";
        field.Type = TType.Bool;
        field.ID = 9;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_offline.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_readonly) {
        field.Name = "is_readonly";
        field.Type = TType.Bool;
        field.ID = 10;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_readonly.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_shadowcopy) {
        field.Name = "is_shadowcopy";
        field.Type = TType.Bool;
        field.ID = 11;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_shadowcopy.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.is_system) {
        field.Name = "is_system";
        field.Type = TType.Bool;
        field.ID = 12;
        oprot.WriteFieldBegin(field);
        oprot.WriteBool(Is_system.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.mbr_type) {
        field.Name = "mbr_type";
        field.Type = TType.I16;
        field.ID = 13;
        oprot.WriteFieldBegin(field);
        oprot.WriteI16(Mbr_type.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.offset) {
        field.Name = "offset";
        field.Type = TType.I64;
        field.ID = 14;
        oprot.WriteFieldBegin(field);
        oprot.WriteI64(Offset.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.partition_number) {
        field.Name = "partition_number";
        field.Type = TType.I32;
        field.ID = 15;
        oprot.WriteFieldBegin(field);
        oprot.WriteI32(Partition_number.Value);
        oprot.WriteFieldEnd();
      }
      if (__isset.size) {
        field.Name = "size";
        field.Type = TType.I64;
        field.ID = 16;
        oprot.WriteFieldBegin(field);
        oprot.WriteI64(Size.Value);
        oprot.WriteFieldEnd();
      }
      oprot.WriteFieldStop();
      oprot.WriteStructEnd();
    }

    public override string ToString() {
      StringBuilder __sb = new StringBuilder("partition_info(");
      bool __first = true;
      if (Access_paths != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Access_paths: ");
        __sb.Append(Access_paths);
      }
      if (__isset.disk_number) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Disk_number: ");
        __sb.Append(Disk_number);
      }
      if (Drive_letter != null && __isset.drive_letter) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Drive_letter: ");
        __sb.Append(Drive_letter);
      }
      if (Gpt_type != null && __isset.gpt_type) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Gpt_type: ");
        __sb.Append(Gpt_type);
      }
      if (Guid != null && __isset.guid) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Guid: ");
        __sb.Append(Guid);
      }
      if (__isset.is_active) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_active: ");
        __sb.Append(Is_active);
      }
      if (__isset.is_boot) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_boot: ");
        __sb.Append(Is_boot);
      }
      if (__isset.is_hidden) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_hidden: ");
        __sb.Append(Is_hidden);
      }
      if (__isset.is_offline) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_offline: ");
        __sb.Append(Is_offline);
      }
      if (__isset.is_readonly) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_readonly: ");
        __sb.Append(Is_readonly);
      }
      if (__isset.is_shadowcopy) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_shadowcopy: ");
        __sb.Append(Is_shadowcopy);
      }
      if (__isset.is_system) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Is_system: ");
        __sb.Append(Is_system);
      }
      if (__isset.mbr_type) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Mbr_type: ");
        __sb.Append(Mbr_type);
      }
      if (__isset.offset) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Offset: ");
        __sb.Append(Offset);
      }
      if (__isset.partition_number) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Partition_number: ");
        __sb.Append(Partition_number);
      }
      if (__isset.size) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Size: ");
        __sb.Append(Size);
      }
      __sb.Append(")");
      return __sb.ToString();
    }

  }

}
