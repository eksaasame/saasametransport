from zope.interface import implementer

from pyramid.interfaces import IRequest, IRequestFactory
from pyramid.util import InstancePropertyMixin
from pyramid.request import CallbackMethodsMixin
from pyramid.decorator import reify

from pyramid_zodbconn import get_connection

from .lib import cli
from .db.store import Store
from .response import Response


@implementer(IRequestFactory)
class RequestFactory():
    def __call__(environ):
        """ Return an instance of ``pyramid.request.Request``"""
        return Request(environ)

    def blank(path):
        return Request({})


@implementer(IRequest)
class Request(InstancePropertyMixin,
              CallbackMethodsMixin,
              ):

    ResponseClass = Response

    def __init__(self, environ):
        self.__dict__['environ'] = environ

    @reify
    def store(self):
        return Store(settings=self.registry.settings)

    @reify
    def dbroot(self):
        return get_connection(self).root()

    @reify
    def response(self):
        return Response()


def create_request_from_environ(environ):
    if 'PATH_INFO' not in environ:
        environ['PATH_INFO'] = ""
        environ['SERVER_PROTOCOL'] = ""
    request = Request(environ=environ)
    if not environ.get('PATH_INFO'):
        environ['PATH_INFO'] = ""
        environ['SERVER_PROTOCOL'] = ""
        request.scheme = request.method = "CONSOLE"
    request.registry = environ['registry']
    request.host = request.registry.settings['launcher.host']
    cli.adopt_by(request.registry.settings)
    return request
