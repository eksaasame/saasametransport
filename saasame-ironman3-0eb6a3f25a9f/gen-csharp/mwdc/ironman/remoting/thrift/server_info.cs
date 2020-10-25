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
  public partial class server_info : TBase
  {
    private string _initiator_name;
    private string _server_id;
    private string _server_name;

    [DataMember(Order = 0)]
    public string Initiator_name
    {
      get
      {
        return _initiator_name;
      }
      set
      {
        __isset.initiator_name = true;
        this._initiator_name = value;
      }
    }

    [DataMember(Order = 0)]
    public string Server_id
    {
      get
      {
        return _server_id;
      }
      set
      {
        __isset.server_id = true;
        this._server_id = value;
      }
    }

    [DataMember(Order = 0)]
    public string Server_name
    {
      get
      {
        return _server_name;
      }
      set
      {
        __isset.server_name = true;
        this._server_name = value;
      }
    }

    [DataMember(Order = 0)]
    public THashSet<network_info> Network_infos { get; set; }


    [XmlIgnore] // XmlSerializer
    [DataMember(Order = 1)]  // XmlObjectSerializer, DataContractJsonSerializer, etc.
    public Isset __isset;
    #if !SILVERLIGHT
    [Serializable]
    #endif
    [DataContract]
    public struct Isset {
      [DataMember]
      public bool initiator_name;
      [DataMember]
      public bool server_id;
      [DataMember]
      public bool server_name;
    }

    #region XmlSerializer support

    public bool ShouldSerializeInitiator_name()
    {
      return __isset.initiator_name;
    }

    public bool ShouldSerializeServer_id()
    {
      return __isset.server_id;
    }

    public bool ShouldSerializeServer_name()
    {
      return __isset.server_name;
    }

    #endregion XmlSerializer support

    public server_info() {
      this._initiator_name = "";
      this.__isset.initiator_name = true;
      this._server_id = "";
      this.__isset.server_id = true;
      this._server_name = "";
      this.__isset.server_name = true;
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
              Initiator_name = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 2:
            if (field.Type == TType.String) {
              Server_id = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 3:
            if (field.Type == TType.String) {
              Server_name = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 4:
            if (field.Type == TType.Set) {
              {
                Network_infos = new THashSet<network_info>();
                TSet _set37 = iprot.ReadSetBegin();
                for( int _i38 = 0; _i38 < _set37.Count; ++_i38)
                {
                  network_info _elem39;
                  _elem39 = new network_info();
                  _elem39.Read(iprot);
                  Network_infos.Add(_elem39);
                }
                iprot.ReadSetEnd();
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
      TStruct struc = new TStruct("server_info");
      oprot.WriteStructBegin(struc);
      TField field = new TField();
      if (Initiator_name != null && __isset.initiator_name) {
        field.Name = "initiator_name";
        field.Type = TType.String;
        field.ID = 1;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Initiator_name);
        oprot.WriteFieldEnd();
      }
      if (Server_id != null && __isset.server_id) {
        field.Name = "server_id";
        field.Type = TType.String;
        field.ID = 2;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Server_id);
        oprot.WriteFieldEnd();
      }
      if (Server_name != null && __isset.server_name) {
        field.Name = "server_name";
        field.Type = TType.String;
        field.ID = 3;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Server_name);
        oprot.WriteFieldEnd();
      }
      if (Network_infos != null) {
        field.Name = "network_infos";
        field.Type = TType.Set;
        field.ID = 4;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteSetBegin(new TSet(TType.Struct, Network_infos.Count));
          foreach (network_info _iter40 in Network_infos)
          {
            _iter40.Write(oprot);
          }
          oprot.WriteSetEnd();
        }
        oprot.WriteFieldEnd();
      }
      oprot.WriteFieldStop();
      oprot.WriteStructEnd();
    }

    public override string ToString() {
      StringBuilder __sb = new StringBuilder("server_info(");
      bool __first = true;
      if (Initiator_name != null && __isset.initiator_name) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Initiator_name: ");
        __sb.Append(Initiator_name);
      }
      if (Server_id != null && __isset.server_id) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Server_id: ");
        __sb.Append(Server_id);
      }
      if (Server_name != null && __isset.server_name) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Server_name: ");
        __sb.Append(Server_name);
      }
      if (Network_infos != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Network_infos: ");
        __sb.Append(Network_infos);
      }
      __sb.Append(")");
      return __sb.ToString();
    }

  }

}