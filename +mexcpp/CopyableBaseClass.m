classdef (Abstract) CopyableBaseClass < matlab.mixin.Copyable
%mexcpp.CopyableBaseClass   Copyable base Matlab class to wrap C++ class instance
%
%   An abstract base class to be paird with the mexObjectHandler() template function
%   to wrap a C++ backend class instance. This class is near identical to 
%   mexcpp.BaseClass, with 2 changes:
%
%   * Based on matlab.mixin.Copyable class to enable deep copy of handle objects
%   * Overrides copyElement protected method of matlab.mixin.Copyable to make
%     a copy of source object.
%
%   The derived classes shall further interact with mexfcn using the action and 
%   static action calls:
%
%      varargout = obj.mexfcn(obj, 'command', varargin) - backend action
%      varargout = obj.mexfcn('command', varargin)      - backend static action
%
%   See also mexcpp.BaseClass include/mexObjectHandler.h
   
   properties (Access = protected, Hidden, NonCopyable, Transient)
      backend % Handle to the backend C++ class instance
   end
   
   methods (Access = protected, Static, Hidden, Abstract)
      varargout = mexfcn(varargin) % mexCopyableObjectHandler mex function, defined by derived class
   end
   
   methods
      function obj = CopyableBaseClass(varargin)
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
   methods(Access = protected)
      % Override copyElement method:
      function cpObj = copyElement(obj)
         % Make a shallow copy of all four properties
         cpObj = copyElement@matlab.mixin.Copyable(obj);

         % Make a deep copy of the C++ object
         obj.mexfcn(obj, 'copy', cpObj);
      end
   end
end
