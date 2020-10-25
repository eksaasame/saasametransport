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
  public partial class cluster_group : TBase
  {
    private string _group_id;
    private string _group_name;
    private string _group_owner;

    [DataMember(Order = 0)]
    public string Group_id
    {
      get
      {
        return _group_id;
      }
      set
      {
        __isset.group_id = true;
        this._group_id = value;
      }
    }

    [DataMember(Order = 0)]
    public string Group_name
    {
      get
      {
        return _group_name;
      }
      set
      {
        __isset.group_name = true;
        this._group_name = value;
      }
    }

    [DataMember(Order = 0)]
    public string Group_owner
    {
      get
      {
        return _group_owner;
      }
      set
      {
        __isset.group_owner = true;
        this._group_owner = value;
      }
    }

    [DataMember(Order = 0)]
    public THashSet<disk_info> Cluster_disks { get; set; }

    [DataMember(Order = 0)]
    public THashSet<volume_info> Cluster_partitions { get; set; }

    [DataMember(Order = 0)]
    public THashSet<cluster_network> Cluster_network_infos { get; set; }


    [XmlIgnore] // XmlSerializer
    [DataMember(Order = 1)]  // XmlObjectSerializer, DataContractJsonSerializer, etc.
    public Isset __isset;
    #if !SILVERLIGHT
    [Serializable]
    #endif
    [DataContract]
    public struct Isset {
      [DataMember]
      public bool group_id;
      [DataMember]
      public bool group_name;
      [DataMember]
      public bool group_owner;
    }

    #region XmlSerializer support

    public bool ShouldSerializeGroup_id()
    {
      return __isset.group_id;
    }

    public bool ShouldSerializeGroup_name()
    {
      return __isset.group_name;
    }

    public bool ShouldSerializeGroup_owner()
    {
      return __isset.group_owner;
    }

    #endregion XmlSerializer support

    public cluster_group() {
      this._group_id = "";
      this.__isset.group_id = true;
      this._group_name = "";
      this.__isset.group_name = true;
      this._group_owner = "";
      this.__isset.group_owner = true;
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
              Group_id = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 2:
            if (field.Type == TType.String) {
              Group_name = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 3:
            if (field.Type == TType.String) {
              Group_owner = iprot.ReadString();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 4:
            if (field.Type == TType.Set) {
              {
                Cluster_disks = new THashSet<disk_info>();
                TSet _set45 = iprot.ReadSetBegin();
                for( int _i46 = 0; _i46 < _set45.Count; ++_i46)
                {
                  disk_info _elem47;
                  _elem47 = new disk_info();
                  _elem47.Read(iprot);
                  Cluster_disks.Add(_elem47);
                }
                iprot.ReadSetEnd();
              }
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 5:
            if (field.Type == TType.Set) {
              {
                Cluster_partitions = new THashSet<volume_info>();
                TSet _set48 = iprot.ReadSetBegin();
                for( int _i49 = 0; _i49 < _set48.Count; ++_i49)
                {
                  volume_info _elem50;
                  _elem50 = new volume_info();
                  _elem50.Read(iprot);
                  Cluster_partitions.Add(_elem50);
                }
                iprot.ReadSetEnd();
              }
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 6:
            if (field.Type == TType.Set) {
              {
                Cluster_network_infos = new THashSet<cluster_network>();
                TSet _set51 = iprot.ReadSetBegin();
                for( int _i52 = 0; _i52 < _set51.Count; ++_i52)
                {
                  cluster_network _elem53;
                  _elem53 = new cluster_network();
                  _elem53.Read(iprot);
                  Cluster_network_infos.Add(_elem53);
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
      TStruct struc = new TStruct("cluster_group");
      oprot.WriteStructBegin(struc);
      TField field = new TField();
      if (Group_id != null && __isset.group_id) {
        field.Name = "group_id";
        field.Type = TType.String;
        field.ID = 1;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Group_id);
        oprot.WriteFieldEnd();
      }
      if (Group_name != null && __isset.group_name) {
        field.Name = "group_name";
        field.Type = TType.String;
        field.ID = 2;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Group_name);
        oprot.WriteFieldEnd();
      }
      if (Group_owner != null && __isset.group_owner) {
        field.Name = "group_owner";
        field.Type = TType.String;
        field.ID = 3;
        oprot.WriteFieldBegin(field);
        oprot.WriteString(Group_owner);
        oprot.WriteFieldEnd();
      }
      if (Cluster_disks != null) {
        field.Name = "cluster_disks";
        field.Type = TType.Set;
        field.ID = 4;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteSetBegin(new TSet(TType.Struct, Cluster_disks.Count));
          foreach (disk_info _iter54 in Cluster_disks)
          {
            _iter54.Write(oprot);
          }
          oprot.WriteSetEnd();
        }
        oprot.WriteFieldEnd();
      }
      if (Cluster_partitions != null) {
        field.Name = "cluster_partitions";
        field.Type = TType.Set;
        field.ID = 5;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteSetBegin(new TSet(TType.Struct, Cluster_partitions.Count));
          foreach (volume_info _iter55 in Cluster_partitions)
          {
            _iter55.Write(oprot);
          }
          oprot.WriteSetEnd();
        }
        oprot.WriteFieldEnd();
      }
      if (Cluster_network_infos != null) {
        field.Name = "cluster_network_infos";
        field.Type = TType.Set;
        field.ID = 6;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteSetBegin(new TSet(TType.Struct, Cluster_network_infos.Count));
          foreach (cluster_network _iter56 in Cluster_network_infos)
          {
            _iter56.Write(oprot);
          }
          oprot.WriteSetEnd();
        }
        oprot.WriteFieldEnd();
      }
      oprot.WriteFieldStop();
      oprot.WriteStructEnd();
    }

    public override string ToString() {
      StringBuilder __sb = new StringBuilder("cluster_group(");
      bool __first = true;
      if (Group_id != null && __isset.group_id) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Group_id: ");
        __sb.Append(Group_id);
      }
      if (Group_name != null && __isset.group_name) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Group_name: ");
        __sb.Append(Group_name);
      }
      if (Group_owner != null && __isset.group_owner) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Group_owner: ");
        __sb.Append(Group_owner);
      }
      if (Cluster_disks != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Cluster_disks: ");
        __sb.Append(Cluster_disks);
      }
      if (Cluster_partitions != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Cluster_partitions: ");
        __sb.Append(Cluster_partitions);
      }
      if (Cluster_network_infos != null) {
        if(!__first) { __sb.Append(", "); }
        __first = false;
        __sb.Append("Cluster_network_infos: ");
        __sb.Append(Cluster_network_infos);
      }
      __sb.Append(")");
      return __sb.ToString();
    }

  }

}
