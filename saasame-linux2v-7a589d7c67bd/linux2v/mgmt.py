from thriftpy.http import client_context


from .spec import thrift


def call_mgmt_service(host, port, service_method, scheme, *args):
    if scheme == 'https':
        import ssl
        ssl_context_factory = ssl._create_unverified_context
    else:
        ssl_context_factory = None

    with client_context(thrift.management_service,
                        host, port, thrift.MANAGEMENT_SERVICE_PATH,
                        scheme=scheme,
                        ssl_context_factory=ssl_context_factory,
                        timeout=180000) as client:
        method = getattr(client, service_method)
        return method(*args)


def main():
    from .spec import GUID_LENGTH_STRING
    # host = "192.168.31.119"
    host = "192.168.31.119"
    port = 443
    session_id = "1"
    # job_id = "FF780CB5-03D6-4971-BDAD-860D248EB1A3"
    try:
        res = call_mgmt_service(
            host, port, "check_snapshots", "https",
            session_id, GUID_LENGTH_STRING)
    except thrift.invalid_operation as ouch:
        print()
        print("What_op:", ouch.what_op)
        print("Why:", ouch.why)

    from pprint import pprint
    pprint(res)


if __name__ == "__main__":
    main()
