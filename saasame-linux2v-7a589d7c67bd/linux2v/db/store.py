import transaction
from BTrees.OOBTree import OOBTree

from ..lib import log


DEFAULT_DB = [
    'jobs',
    'users',
    'sessions'
]


def after_transaction_commit(status, func, *args):

    def after_commit(x, *args):
        func.schedule(args=args, delay=1)

    def after_commit_success(success, *args):
        if success:
            func.schedule(args=args, delay=1)

    def after_commit_failure(success, *args):
        if not success:
            func.schedule(args=args, delay=1)

    if status == "success":
        transaction.get().addAfterCommitHook(after_commit_success, args=args)
    elif status == "failure":
        transaction.get().addAfterCommitHook(after_commit_failure, args=args)
    elif status == "always":
        transaction.get().addAfterCommitHook(after_commit, args=args)

transaction.after_commit = after_transaction_commit


class Store:

    def __init__(self, settings=None, store_connection_settings=None):
        self.settings = settings or {}
        self.store_connection_settings = store_connection_settings

    def force_reset_db(self, dbroot):
        for db in DEFAULT_DB:
            dbroot[db] = OOBTree()

    def initdb(self, dbroot):
        for db in DEFAULT_DB:
            if db not in dbroot:
                dbroot[db] = OOBTree()
        log.debug("Initialized store db")
