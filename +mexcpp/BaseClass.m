classdef (Abstract) BaseClass < handle
%mexcpp.BaseClass   Base Matlab class to wrap C++ class instance
%
%   An abstract base class to be paird with the mexObjectHandler() template function
%   to wrap a C++ backend class instance. The MEX function is declared as 'mexfcn'
%   static abstract method. Derived classes must provide the MEX function as its
%   protected method (also set its attribute to Static and Hidden). Moreover, the MEX
%   function must reside in the derived classes folder (e.g., @derivedClass if the
%   class name is "derivedClass"). 
%
%   Also, the C++ object handle associated with mexfcn is stored in 'backend'
%   property throughout the life of the class object. This property is intended to
%   be accessed by the MEX function ONLY and the derived class should not change it
%   at any time.
%
%   The derived classes shall further interact with mexfcn using the action and 
%   static action calls:
%
%      varargout = obj.mexfcn(obj, 'command', varargin) - backend action
%      varargout = obj.mexfcn('command',varargin)       - backend static action
%
%   It is also permissible to use this file as a template for a user's own classdef
%   instead of using it as a base class. For such use, keep backend property intact
%   as is (directly accessed by mexObjectHandler) and match mexfcn with the compiled 
%   MEX function name
%
%   See also include/mexObjectHandler.h
   
   properties (Access = protected, Hidden, NonCopyable, Transient)
      backend % Handle to the backend C++ class instance
   end
   
   methods (Access = protected, Static, Hidden, Abstract)
      varargout = mexfcn(varargin) % mex function, defined by derived class
   end
   
   methods
      function obj = BaseClass(varargin)
         % instantiate mex backend
         obj.mexfcn(obj, varargin{:});
      end
      
      function delete(obj)
         % destroy the mex backend
         if ~isempty(obj.backend)
            obj.mexfcn(obj, 'delete');
         end
      end
   end
end
