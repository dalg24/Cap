from pycap import PropertyTree, Observer, Observable, Experiment
import unittest


class ObserverObservableTestCase(unittest.TestCase):

    def test_builders(self):
        for AbstractClass in [Observer, Observable]:
            # AbstractClass takes a PropertyTree as argument.
            self.assertRaises(TypeError, AbstractClass)

            # The PropertyTree must specify what concrete class derived from
            # AbstractClass to instantiate.
            ptree = PropertyTree()
            self.assertRaises(KeyError, AbstractClass, ptree)

            # The derived concrete class must be registerd in the dictionary
            # that holds the builders.
            ptree.put_string('type', 'Invalid')
            self.assertRaises(KeyError, AbstractClass, ptree)

            # Now declare a concrete class.
            class ConcreteClass(AbstractClass):

                def __new__(cls, *args, **kwargs):
                    return object.__new__(ConcreteClass)

                def __init__(*args, **kwargs):
                    pass
            # Here is how to register a derived concrete class to the base
            # abstract class.
            AbstractClass._builders['ConcreteClass'] = ConcreteClass

            # Now instantiation works.
            ptree.put_string('type', 'ConcreteClass')
            AbstractClass(ptree)

            # Also can build directly from derived class.
            ConcreteClass()

            # Remove from the dictionary.
            del AbstractClass._builders['ConcreteClass']
            self.assertRaises(KeyError, AbstractClass, ptree)

    def test_update_attach_detach_notify(self):
        # Define an observable.
        class ConcreteObservable(Observable):

            def __new__(cls, *args, **kwargs):
                return object.__new__(ConcreteObservable)

            def __init__(self):
                Observable.__init__(self)
                self._greetings = 'hello world'
        Observable._builders['ConcreteObservable'] = ConcreteObservable

        subject = ConcreteObservable()

        # Define a concrete observer.
        class ConcreteObserver(Observer):

            def __new__(cls, *args, **kwargs):
                return object.__new__(ConcreteObserver)
        Observer._builders['ConcreteObserver'] = ConcreteObserver

        # Derived Observer class need to override the method ``update(...)``.
        observer = ConcreteObserver()
        self.assertRaises(RuntimeError, observer.update, subject)

        class ObserverUpdate(Exception):
            pass

        def update(self, subject, *args, **kwargs):
            print(subject._greetings)
            raise ObserverUpdate
        # Add the method to the definition of ConcreteObserver
        ConcreteObserver.update = update

        # Now it works.
        observer = ConcreteObserver()
        self.assertRaises(ObserverUpdate, observer.update, subject)

        # Attach the observer to the observable.
        subject.attach(observer)

        # An observer may only be attached once.
        self.assertRaises(RuntimeError, subject.attach, observer)

        # Trigger update in the observers.
        self.assertRaises(ObserverUpdate, subject.notify)

        # Detach the observer.
        subject.detach(observer)

        # Only attached observer may be detached.
        self.assertRaises(RuntimeError, subject.detach, observer)

        # Note that the subject only stores a weak reference to its observers.
        subject.attach(observer)
        del observer
        self.assertRaises(RuntimeError, subject.notify)

        # Similarly this will raise an exception.
        subject = ConcreteObservable()
        subject.notify()
        subject.attach(ConcreteObserver())
        self.assertRaises(RuntimeError, subject.notify)


class ExperimentTestCase(unittest.TestCase):

    def test_abstract_class(self):
        # Declare a concrete Experiment
        class DummyExperiment(Experiment):

            def __new__(cls, *args, **kwargs):
                return object.__new__(DummyExperiment)

            def __init__(self, ptree):
                Experiment.__init__(self)
        # Do not forget to register it to the builders dictionary.
        Observable._builders['Dummy'] = DummyExperiment

        # Construct directly via DummyExperiment with a PropertyTree as a
        # positional arguemnt
        ptree = PropertyTree()
        dummy = DummyExperiment(ptree)

        # ... or directly via Experiment by specifying the ``type`` of
        # Experiment.
        ptree.put_string('type', 'Dummy')
        dummy = Experiment(ptree)

        # The method run() must be overloaded.
        self.assertRaises(RuntimeError, dummy.run, None)

        # Override the method run().
        def run(self, device):
            pass
        DummyExperiment.run = run

        # Now calling it without raising an error.
        dummy.run(None)


if __name__ == '__main__':
    unittest.main()
