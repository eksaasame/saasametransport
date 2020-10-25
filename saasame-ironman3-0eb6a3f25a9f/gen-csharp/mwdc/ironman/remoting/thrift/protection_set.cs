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
  public partial class protection_set : TBase
  {
    private string _comment;
    private int _mode;
    private string _site_ids;
    private string _site_name;

    [DataMember(Order = 0)]
    public string Comment
    {
      get
      {
        return _comment;
      }
      set
      {
        __isset.comment = true;
        this._comment = value;
      }
    }

    [DataMember(Order = 0)]
    public int? Mode
    {
      get
      {
        return _mode;
      }
      set
      {
        __isset.mode = value.HasValue;
        if (value.HasValue) this._mode = value.Value;
      }
    }

    [DataMember(Order = 0)]
    public string Site_ids
    {
      get
      {
        return _site_ids;
      }
      set
      {
        __isset.site_ids = true;
        this._site_ids = value;
      }
    }

    [DataMember(Order = 0)]
    public string Site_name
    {
      get
      {
        return _site_name;
      }
      set
      {
        __isset.site_name = true;
        this._site_name = value;
      }
    }

    [DataMember(Order = 0)]
    public Dictionary<string, protection_relationship> Protection_relationships { get; set; }


    [XmlIgnore] // XmlSerializer
    [DataMember(Order = 1)]  // XmlObjectSerializer, DataContractJsonSerializer, etc.
    public Isset __isset;
    #if !SILVERLIGHT
    [Serializable]
    #endif
    [DataContract]
    public struct Isset {
      [DataMember]
      public bool comment;
      [DataMember]
      public bool mode;
      [DataMember]
      public bool site_ids;
      [DataMember]
      public bool site_name;
    }

    #region XmlSerializer support

    public bool ShouldSerializeComment()
    {
      return __isset.comment;
    }

    public bool ShouldSerializeMode()
    {
      return __isset.mode;
    }

    public bool ShouldSerializeSite_ids()
    {
      return __isset.site_ids;
    }

    public bool ShouldSerializeSite_name()
    {
      return __isset.site_name;
    }

    #endregion XmlSerializer support

    public protection_set() {
      this._comment = "";
      this.__isset.comment = true;
      this._mode = -1;
      this.__isset.mode = true;
      this._site_ids = "";
      this.__isset.site_ids = true;
      this._site_name = "";
      this.__isset.site_name = true;
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
            if (field.Type == TType.String) {
              Comment = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 2:
            if (field.Type == TType.I32) {
              Mode = iprot.ReadI32();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 3:
            if (field.Type == TType.String) {
              Site_ids = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 4:
            if (field.Type == TType.String) {
              Site_name = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 5:
            if (field.Type == TType.Map) {
              {
                Protection_relationships = new Dictionary<string, protection_relationship>();
                TMap _map8 = iprot.ReadMapBegin();
                for( int _i9 = 0; _i9 < _map8.Count; ++_i9)
                {
                  string _key10;
                  protection_relationship _val11;
                  _key10 = iprot.ReadString();
                  _val11 = new protection_relationship();
                  _val11.Read(iprot);
                  Protection_relationships[_key10] = _val11;
                }
                iprot.ReadMapEnd();
              }
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
      TStruct struc = new TStruct("protection_set");
      oprot.WriteStructBegin(struc);
      TField field = new TField();
      if (Comment != null && __isset.comment) {
        field.Name = "comment";
        field.Type = TType.String;
        field.ID = 1;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Comment);
        oprot.WriteFieldEnd();
      }
      if (__isset.mode) {
        field.Name = "mode";
        field.Type = TType.I32;
        field.ID = 2;
        oprot.WriteFieldBegin(field);
        oprot.WriteI32(Mode.Value);
        oprot.WriteFieldEnd();
      }
      if (Site_ids != null && __isset.site_ids) {
        field.Name = "site_ids";
        field.Type = TType.String;
        field.ID = 3;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Site_ids);
        oprot.WriteFieldEnd();
      }
      if (Site_name != null && __isset.site_name) {
        field.Name = "site_name";
        field.Type = TType.String;
        field.ID = 4;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Site_name);
        oprot.WriteFieldEnd();
      }
      if (Protection_relationships != null) {
        field.Name = "protection_relationships";
        field.Type = TType.Map;
        field.ID = 5;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteMapBegin(new TMap(TType.String, TType.Struct, Protection_relationships.Count));
          foreach (string _iter12 in Protection_relationships.Keys)
          {
            oprot.WriteString(_iter12);
            Protection_relationships[_iter12].Write(oprot);
          }
          oprot.WriteMapEnd();
        }
        oprot.WriteFieldEnd();
      }
      oprot.WriteFieldStop();
      oprot.WriteStructEnd();
    }

    public override string ToString() {
      StringBuilder __sb = new StringBuilder("protection_set(");
      bool __first = true;
      if (Comment != null && __isset.comment) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Comment: ");
        __sb.Append(Comment);
      }
      if (__isset.mode) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Mode: ");
        __sb.Append(Mode);
      }
      if (Site_ids != null && __isset.site_ids) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Site_ids: ");
        __sb.Append(Site_ids);
      }
      if (Site_name != null && __isset.site_name) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Site_name: ");
        __sb.Append(Site_name);
      }
      if (Protection_relationships != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Protection_relationships: ");
        __sb.Append(Protection_relationships);
      }
      __sb.Append(")");
      return __sb.ToString();
    }

  }

}