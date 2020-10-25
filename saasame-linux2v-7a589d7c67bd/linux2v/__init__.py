"""
NOTE: Importing thrid party library version here will fail package
      requirements check-ing due to we define version here.
"""
version = '1.7.36'


def main(global_config, **settings):
    """Entry point for the application script"""
    from pyramid.config import Configurator
    from .request import RequestFactory
    config = Configurator(settings=settings,
                          request_factory=RequestFactory)
    config.include('pyramid_zodbconn')
    app = config.make_wsgi_app()
    return app
