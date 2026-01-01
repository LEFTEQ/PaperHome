import { useState, useRef, useEffect, KeyboardEvent } from 'react';
import { cn } from '@/lib/utils';

interface EditableTextProps {
  /** Current value */
  value: string;
  /** Called when user saves a new value */
  onSave: (value: string) => Promise<void>;
  /** Additional className for the text/input */
  className?: string;
  /** Maximum character length */
  maxLength?: number;
  /** Placeholder when empty */
  placeholder?: string;
}

/**
 * Inline editable text component.
 * Click to edit, Enter/blur to save, Escape to cancel.
 */
export function EditableText({
  value,
  onSave,
  className,
  maxLength = 100,
  placeholder = 'Enter name...',
}: EditableTextProps) {
  const [isEditing, setIsEditing] = useState(false);
  const [editValue, setEditValue] = useState(value);
  const [isSaving, setIsSaving] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);

  // Update edit value when value prop changes
  useEffect(() => {
    if (!isEditing) {
      setEditValue(value);
    }
  }, [value, isEditing]);

  // Focus and select input when entering edit mode
  useEffect(() => {
    if (isEditing && inputRef.current) {
      inputRef.current.focus();
      inputRef.current.select();
    }
  }, [isEditing]);

  const handleClick = () => {
    if (!isSaving) {
      setIsEditing(true);
    }
  };

  const handleSave = async () => {
    const trimmed = editValue.trim();

    // Don't save if empty or unchanged
    if (!trimmed || trimmed === value) {
      setEditValue(value);
      setIsEditing(false);
      return;
    }

    setIsSaving(true);
    try {
      await onSave(trimmed);
      setIsEditing(false);
    } catch {
      // Revert on error
      setEditValue(value);
    } finally {
      setIsSaving(false);
    }
  };

  const handleCancel = () => {
    setEditValue(value);
    setIsEditing(false);
  };

  const handleKeyDown = (e: KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      handleSave();
    } else if (e.key === 'Escape') {
      e.preventDefault();
      handleCancel();
    }
  };

  const handleBlur = () => {
    // Small delay to allow click events to fire first
    setTimeout(() => {
      if (isEditing) {
        handleSave();
      }
    }, 100);
  };

  if (isEditing) {
    return (
      <input
        ref={inputRef}
        type="text"
        value={editValue}
        onChange={(e) => setEditValue(e.target.value)}
        onKeyDown={handleKeyDown}
        onBlur={handleBlur}
        maxLength={maxLength}
        placeholder={placeholder}
        disabled={isSaving}
        className={cn(
          'bg-transparent border-b-2 border-accent outline-none',
          'text-white font-bold',
          'transition-colors duration-150',
          isSaving && 'opacity-50',
          className
        )}
      />
    );
  }

  return (
    <span
      onClick={handleClick}
      className={cn(
        'cursor-pointer',
        'hover:border-b-2 hover:border-white/30',
        'transition-all duration-150',
        className
      )}
      title="Click to edit"
    >
      {value || placeholder}
    </span>
  );
}
