

class Scheduler:

    def __init__(self, settings=None):
        self.settings = settings or {}
        self.jobs = {}

    def is_job_running(self, job_id):
        return "TODO"

    def is_job_scheduled(self, job_id):
        return "TODO"

    def interrupt_job(self, job_id):
        return "TODO"

    def schedule_job(self, job, triggers=None):
        job_id_jobs = self.jobs.setdefault(job.id, [])
        job_id_jobs.append(job)
