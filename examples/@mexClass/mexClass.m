%mexClass Example MATLAB class wrapper to an underlying C++ class
classdef mexClass < mexcpp.BaseClass
   properties (Dependent,NonCopyable,Transient)
      VarA
      VarB
      VarC
   end
   methods (Access = protected, Static, Hidden)
      varargout = mexfcn(varargin)
   end
   methods (Static)
      function static_fcn()
         mexClass.mexfcn('static_fcn');
      end
   end
   methods
      %% Constructor - Create a new C++ class instance
      function obj = mexClass(varargin)
         obj = obj@mexcpp.BaseClass(varargin{:});
      end
      
      %% Train - an example class method call
      function varargout = train(obj, varargin)
         [varargout{1:nargout}] = obj.mexfcn(obj, 'train', varargin{:});
      end
      
      %% Test - another example class method call
      function varargout = test(obj, varargin)
         [varargout{1:nargout}] = obj.mexfcn(obj, 'test', varargin{:});
      end
      
      function val = get.VarA(obj)
         val = obj.mexfcn(obj,'get','VarA');
      end
      function val = get.VarB(obj)
         val = obj.mexfcn(obj,'get','VarB');
      end
      function val = get.VarC(obj)
         val = obj.mexfcn(obj,'get','VarC');
      end
      
      function set.VarA(obj,val)
         obj.mexfcn(obj,'set','VarA',val);
      end
      function set.VarB(obj,val)
         obj.mexfcn(obj,'set','VarB',val);
      end
      function set.VarC(obj,val)
         obj.mexfcn(obj,'set','VarC',val);
      end
   end
   
   methods (Hidden)
      function B = saveobj(obj)
         B.mexdata = obj.mexfcn(obj, 'save');
      end
   end
   
   methods(Static, Hidden)
      function obj = loadobj(B)
         obj = mexClass();
         obj.mexfcn(obj, 'load', B.mexdata);
      end
   end
end
