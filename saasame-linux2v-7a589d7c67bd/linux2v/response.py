from zope.interface import implementer
from pyramid.interfaces import IResponse


from .spec import thrift


@implementer(IResponse)
class Response:
    error_code = thrift.error_codes.SAASAME_NOERROR
